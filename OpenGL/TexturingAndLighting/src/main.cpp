#include <TextureAndLightingPCH.h>
#include <Camera.h>

using namespace glm;

#define POSITION_ATTRIBUTE 0
#define NORMAL_ATTRIBUTE 2
#define DIFFUSE_ATTRIBUTE 3
#define SPECULAR_ATTRIBUTE 4
#define TEXCOORD0_ATTRIBUTE 8
#define TEXCOORD1_ATTRIBUTE 9
#define TEXCOORD2_ATTRIBUTE 10

#define BUFFER_OFFSET(offset) ((void*)(offset))
#define MEMBER_OFFSET(s,m) ((char*)NULL + (offsetof(s,m)))

int window_width = 700;
int window_height = 500;
int handle = 0;

int g_W, g_A, g_S, g_D, g_Q, g_E;
bool g_bShift = false;

ivec2 g_MousePos;

float light_rotation = 0.0f;

std::clock_t g_PreviousTicks;
std::clock_t g_CurrentTicks;

Camera g_Camera;
vec3 g_InitialCameraPosition;
quat g_InitialCameraRotation;

GLuint g_vaoSphere = 0;
GLuint g_TexturedDiffuseShaderProgram = 0;
GLuint g_SimpleShaderProgram = 0;


GLint g_uniformMVP = -1;
GLint g_uniformModelMatrix = -1;
GLint g_uniformEyePosW = -1;

GLint g_uniformColor = -1;


GLint g_uniformLightPosW = -1;
GLint g_uniformLightColor = -1;
GLint g_uniformAmbient = -1;


GLint g_uniformMaterialEmissive = -1;
GLint g_uniformMaterialDiffuse = -1;
GLint g_uniformMaterialSpecular = -1;
GLint g_uniformMaterialShininess = -1;

GLuint g_EarthTexture = 0;
GLuint snow_texture = 0;
GLuint tree_texture = 0;
GLuint glass_texture = 0;

void IdleGL();
void render();
void KeyboardGL(unsigned char c, int x, int y);
void KeyboardUpGL(unsigned char c, int x, int y);
void SpecialGL(int key, int x, int y);
void SpecialUpGL(int key, int x, int y);
void MouseGL(int button, int state, int x, int y);
void MotionGL(int x, int y);
void ReshapeGL(int w, int h);

#pragma region initialize
//Инициализация
void init_gl(int argc, char* argv[])
{
	glutInit(&argc, argv);

	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

	int screen_w = glutGet(GLUT_SCREEN_WIDTH);
	int screen_h = glutGet(GLUT_SCREEN_HEIGHT);

	glutInitDisplayMode(GLUT_RGBA | GLUT_ALPHA | GLUT_DOUBLE | GLUT_DEPTH);

	glutInitContextVersion(3, 3);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutInitContextFlags(GLUT_FORWARD_COMPATIBLE);

	glutInitWindowPosition((screen_w - window_width) / 2, (screen_h - window_height) / 2);
	glutInitWindowSize(window_width, window_height);

	handle = glutCreateWindow("Individual task 3");

	glutIdleFunc(IdleGL);
	glutDisplayFunc(render);
	glutKeyboardFunc(KeyboardGL);
	glutKeyboardUpFunc(KeyboardUpGL);
	glutSpecialFunc(SpecialGL);
	glutSpecialUpFunc(SpecialUpGL);
	glutMouseFunc(MouseGL);
	glutMotionFunc(MotionGL);
	glutReshapeFunc(ReshapeGL);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
}

void init_GLEW()
{
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		exit(-1);
	}
	 
	if (!GLEW_VERSION_3_3)
	{
		exit(-1);
	}

#ifdef _WIN32
	if (WGLEW_EXT_swap_control)
	{
		wglSwapIntervalEXT(0); 
	}
#endif
}
#pragma endregion

#pragma region shaders
// Loads a shader and returns the compiled shader object.
// If the shader source file could not be opened or compiling the 
// shader fails, then this function returns 0.
GLuint load_shader(GLenum shader_type, const std::string& shader_file)
{
	std::ifstream ifs;
	// Load the shader.
	ifs.open(shader_file);
	if (!ifs)
	{
		std::cerr << "Не возможно открыть файл  \"" << shader_file << "\"" << std::endl;
		return 0;
	}

	std::string source(std::istreambuf_iterator<char>(ifs), (std::istreambuf_iterator<char>()));
	ifs.close();

	// Create a shader object.
	GLuint shader = glCreateShader(shader_type);

	// Load the shader source for each shader object.
	const GLchar* sources[] = { source.c_str() };
	glShaderSource(shader, 1, sources, NULL);

	// Compile the shader.
	glCompileShader(shader);

	// Check for errors
	GLint compileStatus;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
	if (compileStatus != GL_TRUE)
	{
		GLint logLength;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
		GLchar* infoLog = new GLchar[logLength];
		glGetShaderInfoLog(shader, logLength, NULL, infoLog);

#ifdef _WIN32
		OutputDebugString(infoLog);
#else
		std::cerr << infoLog << std::endl;
#endif
		delete infoLog;
		return 0;
	}

	return shader;
}

// Create a shader program from a set of compiled shader objects.
GLuint create_program_for_shader(std::vector<GLuint> shaders)
{
	// Create a shader program.
	GLuint program = glCreateProgram();
	// Attach the appropriate shader objects.
	for (GLuint shader : shaders)
	{
		glAttachShader(program, shader);
	}

	// Link the program
	glLinkProgram(program);

	// Check the link status.
	GLint linkStatus;
	glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
	if (linkStatus != GL_TRUE)
	{
		GLint logLength;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
		GLchar* infoLog = new GLchar[logLength];

		glGetProgramInfoLog(program, logLength, NULL, infoLog);

#ifdef _WIN32
		OutputDebugString(infoLog);
#else
		std::cerr << infoLog << std::endl;
#endif

		delete infoLog;
		return 0;
	}

	return program;
}
#pragma endregion

GLuint load_tex(const std::string& file)
{
	GLuint textureID = SOIL_load_OGL_texture(file.c_str(), SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glBindTexture(GL_TEXTURE_2D, 0);

	return textureID;
}

GLuint create_cube()//(float radius, int part, int stacks)
{ 
	using namespace std;  
	int size = 1;
	vector<vec3> vertices = vector<vec3>{
		vec3(0, 0, 0),
		vec3(0, 0, size),
		vec3(0, size, 0),
		vec3(0, size, size),
		vec3(size, 0, 0),
		vec3(size, 0, size),
		vec3(size, size, 0),
		vec3(size, size, size)
	};
	vector<vec3> normals = vector<vec3>{
		vec3(-1, -1, -1),
		vec3(-1, -1,  1),
		vec3(-1,  1, -1),
		vec3(-1,  1,  1),
		vec3(1, -1, -1),
		vec3(1, -1,  1),
		vec3(1,  1, -1),
		vec3(1,  1,  1)
	};
	vector<vec2> textures = vector<vec2>{
		vec2(1.0, 0.0), //0
		vec2(0.0, 0.0), //1
		vec2(1.0, 1.0), //2
		vec2(0.0, 1.0), //3
		vec2(0.0, 1.0), //4
		vec2(1.0, 1.0), //5
		vec2(0.0, 0.0), //6
		vec2(1.0, 0.0)  //7
	}; 

	vector<GLuint> indicies = vector<GLuint>{
		0, 4, 6, 2,
		1, 5, 7, 3,
		4, 6, 7, 5,
		0, 2, 3, 1,
		0, 4, 5, 1,
		2, 6, 7, 3
	};

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint vbos[4];
	glGenBuffers(4, vbos);

	glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vec3), vertices.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(POSITION_ATTRIBUTE, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(POSITION_ATTRIBUTE);

	glBindBuffer(GL_ARRAY_BUFFER, vbos[1]);
	glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(vec3), normals.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(NORMAL_ATTRIBUTE, 3, GL_FLOAT, GL_TRUE, 0, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(NORMAL_ATTRIBUTE);

	glBindBuffer(GL_ARRAY_BUFFER, vbos[2]);
	glBufferData(GL_ARRAY_BUFFER, textures.size() * sizeof(vec2), textures.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(TEXCOORD0_ATTRIBUTE, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(TEXCOORD0_ATTRIBUTE);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos[3]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicies.size() * sizeof(GLuint), indicies.data(), GL_STATIC_DRAW);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);



	return vao;
}

GLuint create_sphere(float radius, int part, int stacks)
{
	using namespace glm;
	using namespace std;

	const float pi = 3.1415926535897932384626433832795f;
	const float _2pi = 2.0f * pi;

	vector<vec3> vertices;
	vector<vec3> normals;
	vector<vec2> textures;

	for (int i = 0; i <= stacks; ++i)
	{
		// V texture coordinate.
		float V = i / (float)stacks;
		float phi = V * pi;

		for (int j = 0; j <= part; ++j)
		{
			// U texture coordinate.
			float U = j / (float)part;
			float theta = U * _2pi;

			float X = cos(theta) * sin(phi);
			float Y = cos(phi);
			float Z = sin(theta) * sin(phi);

			vertices.push_back(vec3(X, Y, Z) * radius);
			normals.push_back(vec3(X, Y, Z));
			textures.push_back(vec2(U, V));
		}
	}

	// Now generate the index buffer
	vector<GLuint> indicies;

	for (int i = 0; i < part * stacks + part; ++i)
	{
		indicies.push_back(i);
		indicies.push_back(i + part + 1);
		indicies.push_back(i + part);

		indicies.push_back(i + part + 1);
		indicies.push_back(i);
		indicies.push_back(i + 1);
	}

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint vbos[4];
	glGenBuffers(4, vbos);

	glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vec3), vertices.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(POSITION_ATTRIBUTE, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(POSITION_ATTRIBUTE);

	glBindBuffer(GL_ARRAY_BUFFER, vbos[1]);
	glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(vec3), normals.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(NORMAL_ATTRIBUTE, 3, GL_FLOAT, GL_TRUE, 0, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(NORMAL_ATTRIBUTE);

	glBindBuffer(GL_ARRAY_BUFFER, vbos[2]);
	glBufferData(GL_ARRAY_BUFFER, textures.size() * sizeof(vec2), textures.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(TEXCOORD0_ATTRIBUTE, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(TEXCOORD0_ATTRIBUTE);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos[3]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicies.size() * sizeof(GLuint), indicies.data(), GL_STATIC_DRAW);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	return vao;
}

int main(int argc, char* argv[])
{
	g_PreviousTicks = std::clock();

	g_InitialCameraPosition = vec3(0, 0, 100);
	g_Camera.SetPosition(g_InitialCameraPosition);
	g_Camera.SetRotation(g_InitialCameraRotation);

	init_gl(argc, argv);
	init_GLEW();

	snow_texture = load_tex("../data/Textures/snow.jpg");
	tree_texture = load_tex("../data/Textures/tree.jpg");
	glass_texture = load_tex("../data/Textures/glass.jpg");
	GLuint vertexShader = load_shader(GL_VERTEX_SHADER, "../data/shaders/simpleShader.vert");
	GLuint fragmentShader = load_shader(GL_FRAGMENT_SHADER, "../data/shaders/simpleShader.frag");

	std::vector<GLuint> shaders;
	shaders.push_back(vertexShader);
	shaders.push_back(fragmentShader);

	g_SimpleShaderProgram = create_program_for_shader(shaders);
	assert(g_SimpleShaderProgram);

	// Set the color uniform variable in the simple shader program to white.
	g_uniformColor = glGetUniformLocation(g_SimpleShaderProgram, "color");

	vertexShader = load_shader(GL_VERTEX_SHADER, "../data/shaders/texturedDiffuse.vert");
	fragmentShader = load_shader(GL_FRAGMENT_SHADER, "../data/shaders/texturedDiffuse.frag");

	shaders.clear();

	shaders.push_back(vertexShader);
	shaders.push_back(fragmentShader);
	g_TexturedDiffuseShaderProgram = create_program_for_shader(shaders);
	assert(g_TexturedDiffuseShaderProgram);

	g_uniformMVP = glGetUniformLocation(g_TexturedDiffuseShaderProgram, "ModelViewProjectionMatrix");
	g_uniformModelMatrix = glGetUniformLocation(g_TexturedDiffuseShaderProgram, "ModelMatrix");
	g_uniformEyePosW = glGetUniformLocation(g_TexturedDiffuseShaderProgram, "EyePosW");

	// Light properties.
	g_uniformLightPosW = glGetUniformLocation(g_TexturedDiffuseShaderProgram, "LightPosW");
	g_uniformLightColor = glGetUniformLocation(g_TexturedDiffuseShaderProgram, "LightColor");

	// Global ambient.
	g_uniformAmbient = glGetUniformLocation(g_TexturedDiffuseShaderProgram, "Ambient");

	// Material properties.
	g_uniformMaterialEmissive = glGetUniformLocation(g_TexturedDiffuseShaderProgram, "MaterialEmissive");
	g_uniformMaterialDiffuse = glGetUniformLocation(g_TexturedDiffuseShaderProgram, "MaterialDiffuse");
	g_uniformMaterialSpecular = glGetUniformLocation(g_TexturedDiffuseShaderProgram, "MaterialSpecular");
	g_uniformMaterialShininess = glGetUniformLocation(g_TexturedDiffuseShaderProgram, "MaterialShininess");

	glutMainLoop();
}

void ReshapeGL(int w, int h)
{
	if (h == 0)
	{
		h = 1;
	}

	window_width = w;
	window_height = h;

	g_Camera.SetViewport(0, 0, w, h);
	g_Camera.SetProjectionRH(30.0f, w / (float)h, 0.1f, 200.0f);

	glutPostRedisplay();
}

void DrawBallsVert(std::vector<int> ys, std::vector<float> sizes, std::vector<int> xs = std::vector<int>{ 0, 0 }, std::vector<int> zs =
	std::vector<int>{ 0, 0 }) {
	const vec4 white(1);
	const vec4 black(0);
	const vec4 ambient(0.1f, 0.1f, 0.1f, 1.0f);
	int slices = 32;
	int stacks = 32;
	int numIndicies = (slices * stacks + slices) * 6;
	mat4 modelMatrix = rotate(radians(light_rotation), vec3(0, -1, 0)) * translate(vec3(90, 0, 0));
	mat4 mvp = g_Camera.GetProjectionMatrix() * g_Camera.GetViewMatrix() * modelMatrix;
	for (int i = 0; i < ys.size(); ++i) {
		
		modelMatrix = translate(vec3(xs[i], ys[i], zs[i])) * scale(vec3(sizes[i]));
		mvp = g_Camera.GetProjectionMatrix() * g_Camera.GetViewMatrix() * modelMatrix;

		glUniformMatrix4fv(g_uniformMVP, 1, GL_FALSE, value_ptr(mvp));
		glUniformMatrix4fv(g_uniformModelMatrix, 1, GL_FALSE, value_ptr(modelMatrix));
		glDrawElements(GL_TRIANGLES, numIndicies, GL_UNSIGNED_INT, BUFFER_OFFSET(0));
	}
}

int light_x = 20;
int light_y = 30;
int light_z = 30;
GLint g_vaoCube;
void render()
{
	int slices = 32;
	int stacks = 32;
	int numIndicies =  (slices * stacks + slices) * 6;
	if (g_vaoSphere == 0)
	{
		g_vaoSphere = create_sphere(1, slices, stacks);
		g_vaoCube = create_cube();
	}

	const vec4 white(1);
	const vec4 black(0);
	const vec4 ambient(0.5f, 0.5f, 0.5f, 1.0f);
	const vec4 green(0.2f, 0.4f, 0.2f, 1.0f);
	const vec4 light_color(1.0f, 0.7f, 0.7f, 1.0f);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBindVertexArray(g_vaoSphere);

	glUseProgram(g_SimpleShaderProgram); 
	mat4 modelMatrix = rotate(radians(light_rotation), vec3(10, 3, 0)) * translate(vec3(light_x, light_y, light_z));
	mat4 mvp = g_Camera.GetProjectionMatrix() * g_Camera.GetViewMatrix() * modelMatrix;
	GLuint uniformMVP = glGetUniformLocation(g_SimpleShaderProgram, "MVP");
	glUniformMatrix4fv(uniformMVP, 1, GL_FALSE, value_ptr(mvp));
	glUniform4fv(g_uniformColor, 1, value_ptr(light_color));
	glUniform4fv(g_uniformMaterialEmissive, 1, value_ptr(light_color));
	glUniform4fv(g_uniformMaterialDiffuse, 1, value_ptr(white));
	glUniform4fv(g_uniformMaterialSpecular, 1, value_ptr(white));
	glUniform1f(g_uniformMaterialShininess, 0.0f);
	glDrawElements( GL_TRIANGLES, numIndicies, GL_UNSIGNED_INT, BUFFER_OFFSET(0) );
	glUseProgram(g_TexturedDiffuseShaderProgram);

	glBindTexture(GL_TEXTURE_2D, snow_texture);


	glUniform4fv(g_uniformLightPosW, 1, value_ptr(modelMatrix[3]));
	glUniform4fv(g_uniformLightColor, 1, value_ptr(light_color));
	glUniform4fv(g_uniformAmbient, 1, value_ptr(ambient));

	modelMatrix = translate(vec3(0, -1000, 0)) * scale(vec3(1000.756f));
	vec4 eyePosW = vec4(g_Camera.GetPosition(), 1);
	mvp = g_Camera.GetProjectionMatrix() * g_Camera.GetViewMatrix() * modelMatrix;

	glUniformMatrix4fv(g_uniformMVP, 1, GL_FALSE, value_ptr(mvp));
	glUniformMatrix4fv(g_uniformModelMatrix, 1, GL_FALSE, value_ptr(modelMatrix));
	glUniform4fv(g_uniformEyePosW, 1, value_ptr(eyePosW));

	// Material properties.
	glUniform4fv(g_uniformMaterialEmissive, 0, value_ptr(black));
	glUniform4fv(g_uniformMaterialDiffuse, 0, value_ptr(white));
	glUniform4fv(g_uniformMaterialSpecular, 0, value_ptr(white));
	glUniform1f(g_uniformMaterialShininess, 0.0f);

	//glDrawElements(GL_QUADS, 24 * sizeof(GLubyte), GL_UNSIGNED_BYTE, NULL);
	glDrawElements(GL_TRIANGLES, numIndicies, GL_UNSIGNED_INT, BUFFER_OFFSET(0));

	modelMatrix = translate(vec3(50, 0, 50));

	glUniform4fv(g_uniformAmbient, 1, value_ptr(green));
	glBindTexture(GL_TEXTURE_2D, tree_texture);

	glUniform4fv(g_uniformMaterialEmissive, 1, value_ptr(black));
	glUniform4fv(g_uniformMaterialDiffuse, 1, value_ptr(white));
	glUniform4fv(g_uniformMaterialSpecular, 1, value_ptr(white));
	glUniform1f(g_uniformMaterialShininess, 10.0f);
	DrawBallsVert(std::vector<int> {-1, -1, -1, -1, -1,
									2,2, 2, 2,
									5, 5, 5, 
									8, 8, 
									11},
		std::vector<float> ( 15,2.1f), 
		std::vector<int>{-10, -6, -2,2,6,
						-8,-4,0,4,
						-6, -2, 2,
						-4,0,
						-2},
		std::vector<int>(15,60));

	glUniform4fv(g_uniformAmbient, 1, value_ptr(white));
	glBindTexture(GL_TEXTURE_2D, snow_texture);
	glUniform4fv(g_uniformMaterialEmissive, 1, value_ptr(black));
	glUniform4fv(g_uniformMaterialDiffuse, 1, value_ptr(white));
	glUniform4fv(g_uniformMaterialSpecular, 1, value_ptr(white));
	glUniform1f(g_uniformMaterialShininess, 0.0f);
	DrawBallsVert(std::vector<int> {1, 1, 1, 1, 1,
		4, 4, 4, 4,
		7, 7, 7,
		10, 10,
		9},
		std::vector<float>(15, 0.5f),
		std::vector<int>{-10, -6, -2, 2, 6,
		-8, -4, 0, 4,
		-6, -2, 2,
		-4, 0,
		-2},
		std::vector<int>(15, 62));

	glUniform4fv(g_uniformAmbient, 1, value_ptr(white));
	glBindTexture(GL_TEXTURE_2D, snow_texture);
	glUniform4fv(g_uniformMaterialEmissive, 1, value_ptr(black));
	glUniform4fv(g_uniformMaterialDiffuse, 1, value_ptr(white));
	glUniform4fv(g_uniformMaterialSpecular, 1, value_ptr(white));
	glUniform1f(g_uniformMaterialShininess, 100.0f);
	DrawBallsVert(std::vector<int> {1},
		std::vector<float>(15, 5.0f),
		std::vector<int>{ 10},
		std::vector<int>(1, 50));
  
	 
	glBindVertexArray(0);
	glUseProgram(0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glutSwapBuffers();
}

void IdleGL()
{
	g_CurrentTicks = std::clock();
	float deltaTicks = (float)(g_CurrentTicks - g_PreviousTicks);
	g_PreviousTicks = g_CurrentTicks;

	float fDeltaTime = deltaTicks / (float)CLOCKS_PER_SEC;

	float cameraSpeed = 1.0f;
	if (g_bShift)
	{
		cameraSpeed = 5.0f;
	}

	g_Camera.Translate(vec3(g_D - g_A, g_Q - g_E, g_S - g_W) * cameraSpeed * fDeltaTime);

	// Rate of rotation in (degrees) per second
	const float fRotationRate1 = 5.0f;
	const float fRotationRate2 = 12.5f;
	const float fRotationRate3 = 20.0f;


	//g_fSunRotation += fRotationRate3 * fDeltaTime;
	light_rotation = fmod(light_rotation, 360.0f);

	glutPostRedisplay();
}

void KeyboardGL(unsigned char c, int x, int y)
{
	switch (c)
	{
	case 'y':
		light_y += 1;
		break; 
	case 'h':
			light_y-=1;
			break;
	case 'z':
		light_z+=1;
		break;
	case 'x':
		light_z -= 1;
		break;
	case 'w':
	case 'W':
		g_W = 5;
		break;
	case 'a':
	case 'A':
		g_A = 10;
		break;
	case 's':
	case 'S':
		g_S = 10;
		break;
	case 'd':
	case 'D':
		g_D = 5;
		break;
	case 'q':
	case 'Q':
		g_Q = 5;
		break;
	case 'e':
	case 'E':
		g_E = 5;
		break;
	case 'r':
	case 'R':
		g_Camera.SetPosition(g_InitialCameraPosition);
		g_Camera.SetRotation(g_InitialCameraRotation);
		
		light_rotation = 0.0f;
		break;
	case 27:
		glutLeaveMainLoop();
		break;
	}
}

void KeyboardUpGL(unsigned char c, int x, int y)
{
	switch (c)
	{
	case 'w':
	case 'W':
		g_W = 0;
		break;
	case 'a':
	case 'A':
		g_A = 0;
		break;
	case 's':
	case 'S':
		g_S = 0;
		break;
	case 'd':
	case 'D':
		g_D = 0;
		break;
	case 'q':
	case 'Q':
		g_Q = 0;
		break;
	case 'e':
	case 'E':
		g_E = 0;
		break;

	default:
		break;
	}
}

void SpecialGL(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_UP:
		light_rotation -= 2.0F;
		light_rotation = fmod(light_rotation, 360.0f); break;
	case GLUT_KEY_DOWN:
		light_rotation += 2.0F;
		light_rotation = fmod(light_rotation, 360.0f); break;
	case GLUT_KEY_RIGHT: light_x += 1; break;
	case GLUT_KEY_LEFT: light_x -= 1; break;
	case GLUT_KEY_SHIFT_L:
	case GLUT_KEY_SHIFT_R:
	{
		g_bShift = true;
	}
	break;
	}
} 

void SpecialUpGL(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_SHIFT_L:
	case GLUT_KEY_SHIFT_R:
	{
		g_bShift = false;
	}
	break;
	}
}

void MouseGL(int button, int state, int x, int y)
{
	g_MousePos = ivec2(x, y);
}

void MotionGL(int x, int y)
{
	ivec2 mousePos = ivec2(x, y);
	vec2 delta = vec2(mousePos - g_MousePos);
	g_MousePos = mousePos;


	quat rotX = angleAxis<float>(radians(delta.y) * 0.5f, vec3(1, 0, 0));
	quat rotY = angleAxis<float>(radians(delta.x) * 0.5f, vec3(0, 1, 0));

	g_Camera.Rotate( rotX * rotY );
	//    g_Rotation = ( rotX * rotY ) * g_Rotation;

}
