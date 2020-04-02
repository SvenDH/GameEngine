#ifdef VERTEX_PROGRAM
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTex;
layout (location = 2) in vec4 aCol;
layout (location = 3) in uint aIdx;

out vec2 TexCoords;
out vec4 Color;
flat out uint Index;

uniform mat4 projection;

void main() {
	gl_Position = projection * vec4(aPos, 0.0, 1.0);
    TexCoords = aTex;
	Color = aCol;
	Index = aIdx;
}
#endif
#ifdef FRAGMENT_PROGRAM
in vec2 TexCoords;
in vec4 Color;
flat in uint Index;

uniform sampler2DArray image;

void main() {
	vec4 tex = texture(image, vec3(TexCoords.xy, Index));
    gl_FragColor = tex * Color;
}
#endif