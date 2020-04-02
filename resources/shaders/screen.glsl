#ifdef GEOMETRY_PROGRAM

#endif
#ifdef VERTEX_PROGRAM
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTex;

out vec2 TexCoords;

void main() {
	gl_Position = vec4(aPos * 2 - 1, 1.0);
    TexCoords = aTex;
}
#endif
#ifdef FRAGMENT_PROGRAM
in vec2 TexCoords;

uniform sampler2D image;

void main() {
	vec4 color = texture(image, TexCoords);
	if (color.a < 0.1)
		discard;
    gl_FragColor = color;
}  
#endif