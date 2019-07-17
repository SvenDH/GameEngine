#include "data.h"
#include "platform.h"
#include "utils.h"

MemoryPool* pool_init(size_t elementsize, size_t blocksize) {
	MemoryPool* p = malloc(sizeof(MemoryPool));

	p->elementsize_ = max(elementsize, sizeof(free_node));
	p->blocksize_ = blocksize;
	
	pool_free_all(p);

	p->blocksused = POOL_BLOCKS_INITIAL;
	p->blocks = malloc(sizeof(char*)* p->used);

	for (int i = 0; i < p->blocksused; ++i)
		p->blocks[i] = NULL;

	return p;
}

void *pool_alloc(MemoryPool* p) {
	if (p->free_list != NULL) {
		void *recycle = p->free_list;
		p->free_list = p->free_list->next;
		return recycle;
	}

	if (++p->used == p->blocksize_) {
		p->used = 0;
		if (++p->block_i == (int)p->blocksused) {
			p->blocksused <<= 1;
			p->blocks = realloc(p->blocks, sizeof(char*)* p->blocksused);

			for (int i = p->blocksused >> 1; i < p->blocksused; ++i)
				p->blocks[i] = NULL;
		}

		if (p->blocks[p->block_i] == NULL)
			p->blocks[p->block_i] = (char*)aligned_alloc(ALIGNMENT, p->elementsize_ * p->blocksize_);
	}

	return p->blocks[p->block_i] + p->used * p->elementsize_;
}

void pool_free(MemoryPool* p, void* ptr) {
	free_node *free = p->free_list;
	p->free_list = ptr;
	p->free_list->next = free; //Leave a pointer to next free in the element
}

void pool_free_all(MemoryPool* p) {
	p->used = p->blocksize_ - 1;
	p->block_i = -1;
	p->free_list = NULL;
}

void poolFreePool(MemoryPool* p) {
	uint32_t i;
	for (i = 0; i < p->blocksused; ++i) {
		if (p->blocks[i] == NULL)
			break;
		else
			free(p->blocks[i]);
	}

	free(p->blocks);
}

RingBuffer* ringbuffer_init(size_t cap) {
	RingBuffer* rb = malloc(sizeof(RingBuffer));
	rb->data_ = (char*)aligned_alloc(ALIGNMENT, cap);
	rb->max_ = cap;
	rb->head_ = 0;
	rb->tail_ = 0;
	rb->size_ = 0;
	uv_mutex_init(&rb->mutex);
	return rb;
}

size_t buffer_write(RingBuffer* buf, char* data, size_t num) {
	if (num == 0) return 0;
	//TODO: check if there is room for data with semaphore
	uv_mutex_lock(&buf->mutex);
	size_t bytes_to_write = min(num, buf->max_ - buf->size_);

	// Write in a single step
	if (bytes_to_write <= buf->max_ - buf->tail_) {
		memcpy(buf->data_ + buf->tail_, data, bytes_to_write);
		buf->tail_ += bytes_to_write;
		if (buf->tail_ == buf->max_) buf->tail_ = 0;
	}
	else { // Write in two steps
		size_t size_1 = buf->max_ - buf->tail_;
		memcpy(buf->data_ + buf->tail_, data, size_1);
		size_t size_2 = bytes_to_write - size_1;
		memcpy(buf->data_, data + size_1, size_2);
		buf->tail_ = size_2;
	}

	buf->size_ += bytes_to_write;
	uv_mutex_unlock(&buf->mutex);

	return bytes_to_write;
}

size_t buffer_read(RingBuffer* buf, char* data, size_t num) {
	if (num == 0) return 0;
	//TODO: check if there is data with another semaphore
	uv_mutex_lock(&buf->mutex);
	size_t bytes_to_read = min(num, buf->size_);

	// Read in a single step
	if (bytes_to_read <= buf->max_ - buf->head_) {
		memcpy(data, buf->data_ + buf->head_, bytes_to_read);
		buf->head_ += bytes_to_read;
		if (buf->head_ == buf->max_) buf->head_ = 0;
	}
	else { // Read in two steps
		size_t size_1 = buf->max_ - buf->head_;
		memcpy(data, buf->data_ + buf->head_, size_1);
		size_t size_2 = bytes_to_read - size_1;
		memcpy(data + size_1, buf->data_, size_2);
		buf->head_ = size_2;
	}

	buf->size_ -= bytes_to_read;
	uv_mutex_unlock(&buf->mutex);

	return bytes_to_read;
}

int serialize_bytes(RingBuffer* buf, uint8_t* data, int32_t len) {
	assert(len == buffer_write(buf, data, len));
	return 0;
}

int deserialize_bytes(RingBuffer* buf, uint8_t* data, int32_t len) {
	assert(len == buffer_read(buf, data, len));
	return 0;
}

//TODO: check for min/max values
int serialize_int(RingBuffer* buf, int32_t* integer) {
	int32_t sint = htonl(*integer);
	return serialize_bytes(buf, &sint, sizeof(int32_t));
}

int deserialize_int(RingBuffer* buf, int32_t* integer) {
	int r = deserialize_bytes(buf, integer, 4);
	*integer = ntohl(*integer);
	return r;
}

//TODO: check if string fits
int serialize_string(RingBuffer* buf, uint8_t* data, int32_t* len) {
	//assert(buf->max_ - buf->size_ >= 4 + *len);
	int r = serialize_int(buf, len);
	r += serialize_bytes(buf, data, *len);
	return 4 + *len;
}

int deserialize_string(RingBuffer* buf, uint8_t* data, int32_t* len) {
	int r = deserialize_int(buf, len);
	r += deserialize_bytes(buf, data, *len);
	return 4 + *len;
}

HashMap* hashmap_new() {
	HashMap* m = (HashMap*)malloc(sizeof(HashMap));
	if (!m) goto err;

	m->data = (hash_node*)calloc(HASHMAP_INITIAL, sizeof(hash_node));
	if (!m->data) goto err;

	m->max_ = HASHMAP_INITIAL;
	m->size_ = 0;

	return m;
err:
	if (m)
		hashmap_free(m);
	return NULL;
}

int hashmap_hash(HashMap* m, const char* key, size_t len) {
	int curr;
	int i;

	if (m->size_ >= (m->max_ / 2)) return -1;

	curr = hash_int(key, len, m->max_);

	for (i = 0; i < HASHMAP_MAX_CHAIN_LENGTH; i++) {
		if (m->data[curr].in_use == 0)
			return curr;

		if (m->data[curr].in_use == 1 && (strncmp(m->data[curr].key, key, len) == 0))
			return curr;

		curr = (curr + 1) % m->max_;
	}

	return -1;
}

int hashmap_rehash(HashMap* m) {
	int i;
	int old_size;
	hash_node* curr;

	hash_node* temp = (hash_node *) calloc(2 * m->max_, sizeof(hash_node));
	if (!temp) return -1;

	curr = m->data;
	m->data = temp;

	old_size = m->max_;
	m->max_ = 2 * m->max_;
	m->size_ = 0;

	for (i = 0; i < old_size; i++) {
		int status;

		if (curr[i].in_use == 0)
			continue;

		status = hashmap_put(m, curr[i].key, curr[i].len, curr[i].data);
		if (status != 0)
			return status;
	}

	free(curr);

	return 0;
}

int hashmap_put(HashMap* m, const char* key, size_t len, void* value) {
	int index;

	index = hashmap_hash(m, key, len);
	while (index < 0) {
		if (hashmap_rehash(m) == -1) {
			return -1;
		}
		index = hashmap_hash(m, key, len);
	}

	m->data[index].data = value;
	m->data[index].key = key;
	m->data[index].len = len;
	m->data[index].in_use = 1;
	m->size_++;

	return 0;
}

int hashmap_get(HashMap* m, const char* key, size_t len, void**arg) {
	int curr;
	int i;

	curr = hash_int(key, len, m->max_);

	for (i = 0; i < HASHMAP_MAX_CHAIN_LENGTH; i++) {

		int in_use = m->data[curr].in_use;
		if (in_use == 1) {
			if (strncmp(m->data[curr].key, key, len) == 0) {
				*arg = (m->data[curr].data);
				return 0;
			}
		}

		curr = (curr + 1) % m->max_;
	}

	*arg = NULL;
	return -1;
}

int hashmap_iterate(HashMap* m, iter_func f, void* item) {
	int i;

	if (m->size_ <= 0)
		return -1;

	for (i = 0; i < m->max_; i++)
		if (m->data[i].in_use != 0) {
			void* data = (void*)(m->data[i].data);
			int status = f(item, data);
			if (status != 0) {
				return status;
			}
		}

	return 0;
}

int hashmap_remove(HashMap* m, const char* key, size_t len) {
	int i;
	int curr;

	curr = hash_int(key, len, m->max_);

	for (i = 0; i < HASHMAP_MAX_CHAIN_LENGTH; i++) {

		int in_use = m->data[curr].in_use;
		if (in_use == 1) {
			if (strncmp(m->data[curr].key, key, len) == 0) {
				m->data[curr].in_use = 0;
				m->data[curr].data = NULL;
				m->data[curr].key = NULL;
				m->data[curr].len = 0;

				m->size_--;
				return 0;
			}
		}
		curr = (curr + 1) % m->max_;
	}
	return -1;
}

void hashmap_free(HashMap* m) {
	free(m->data);
	free(m);
}
