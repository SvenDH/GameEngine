#ifdef GEOMETRY_PROGRAM

#endif
#ifdef VERTEX_PROGRAM
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTex;

out vec2 TexCoords;

uniform mat4 model;
uniform mat4 projection;

void main() {
	gl_Position = projection * model * vec4(aPos, 1.0);
    TexCoords = aTex;
}
#endif
#ifdef FRAGMENT_PROGRAM
in vec2 TexCoords;

uniform int mapindex;
uniform ivec2 tilesize;
uniform ivec2 mapsize;
uniform sampler2DArray tileset;
uniform sampler2DArray tilemap;

void main() { 
	float index = texture(tilemap, vec3(TexCoords, mapindex)).r * 256;
	float xpos = fract(TexCoords.x * mapsize.x);
	float ypos = fract(TexCoords.y * mapsize.y);
    gl_FragColor = texture(tileset, vec3(xpos, ypos, index));
}
#endif