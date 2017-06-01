R""(
#version 330 core
#define M_PI (3.141592653589793238462643383)
#define MAX_ITER (1000)
#define f1 (3 * 2.0 * M_PI / MAX_ITER)
#define f2 (3 * 2.0 * M_PI / MAX_ITER)
#define f3 (3 * 2.0 * M_PI / MAX_ITER)
#define p1 (0)
#define p2 (2)
#define p3 (4)
in vec2 coord;
out vec4 FragColor;

uniform float scale;
uniform vec2 offset;

void main() {
	float x0 = (coord.x * scale) + offset.x;
	float y0 = (coord.y * scale) + offset.y;
	float x = x0;
	float y = y0;
	int iteration = MAX_ITER;
	float xtemp;

	float q = (x - 0.25f) * (x - 0.25f) + y * y;

	if (!((x + 1) * (x + 1) + y * y < 1 / 16 || q * (q + x - 0.25) < 0.25 * y * y)) {
		for (iteration = 0; x * x + y * y < (1 << 5) && iteration < MAX_ITER; iteration++) {
			xtemp = x * x - y * y + x0;
			y     = 2 * x * y + y0;
			x     = xtemp;
		}
	}

	FragColor = vec4(
				(iteration == MAX_ITER ? 0.0f : 1.0f) * sin(f1 * iteration + p1) + 0.5f,
				(iteration == MAX_ITER ? 0.0f : 1.0f) * sin(f2 * iteration + p2) + 0.5f,
				(iteration == MAX_ITER ? 0.0f : 1.0f) * sin(f3 * iteration + p3) + 0.5f,
				0.0f);
}
)""
