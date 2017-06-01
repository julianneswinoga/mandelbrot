R""(
#version 330 core
in vec2 coord;
out vec4 FragColor;

uniform float scale;
uniform vec2 offset;

void main() {
	float x0 = (coord.x * scale) + offset.x;
	float y0 = (coord.y * scale) + offset.y;
	float x = x0;
	float y = y0;
	int iteration = 1000;
	float xtemp;

	float q = (x - 0.25f) * (x - 0.25f) + y * y;

	if (!((x + 1) * (x + 1) + y * y < 1 / 16 || q * (q + x - 0.25) < 0.25 * y * y)) {
		for (iteration = 0; x * x + y * y < (1 << 10) && iteration < 1000; iteration++) {
			xtemp = x * x - y * y + x0;
			y     = 2 * x * y + y0;
			x     = xtemp;
		}
	}

	FragColor = vec4(iteration / 1000.0f, 0.5f, 0.5f, 1.0f);
}
)""
