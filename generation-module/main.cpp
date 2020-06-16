#include <Windows.h>

#include <iostream>


#include "CameraDriver.h"
#include "setup_config.h"
#include "Color.h"
#include "Shader.h"
#include "Cylinder.h"
#include "camera_definition.h"
#include "Paraboloid.h"
#include "CurvedBlade.h"

extern "C" {
	#include <libavcodec/avcodec.h>
	#include <libavutil/imgutils.h>
	#include <libavutil/opt.h>
	#include <libswscale/swscale.h>
}

#include "utils/json.hpp"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"

#define GLM_ENABLE_EXPERIMENTAL

#include "glm/gtx/spline.hpp"

enum Constants { SCREENSHOT_MAX_FILENAME = 256 };
static GLubyte* pixels = NULL;
static GLuint fbo;
static GLuint rbo_color;
static GLuint rbo_depth;
static const unsigned int HEIGHT = 800;
static const unsigned int WIDTH = 1500;
static int offscreen = 0;
static unsigned int max_nframes = 100;
static unsigned int nframes = 0;
static unsigned int time0;

/* Model. */
static double angle;
static double delta_angle;

/* Adapted from: https://github.com/cirosantilli/cpp-cheat/blob/19044698f91fefa9cb75328c44f7a487d336b541/ffmpeg/encode.c */
static AVCodecContext* c = NULL;
static AVFrame* frame;
static AVPacket pkt;
static FILE* file;
static struct SwsContext* sws_context = NULL;
static uint8_t* rgb = NULL;

static void ffmpeg_encoder_set_frame_yuv_from_rgb(uint8_t* rgb) {
	const int in_linesize[1] = { 4 * c->width };
	sws_context = sws_getCachedContext(sws_context,
		c->width, c->height, AV_PIX_FMT_RGB32,
		c->width, c->height, AV_PIX_FMT_YUV420P,
		0, NULL, NULL, NULL);
	sws_scale(sws_context, (const uint8_t* const*)&rgb, in_linesize, 0,
		c->height, frame->data, frame->linesize);
}

void ffmpeg_encoder_start(const char* filename, AVCodecID codec_id, int fps, int width, int height) {
	AVCodec* codec;
	int ret;
	avcodec_register_all();
	codec = avcodec_find_encoder(codec_id);
	if (!codec) {
		fprintf(stderr, "Codec not found\n");
		exit(1);
	}
	c = avcodec_alloc_context3(codec);
	if (!c) {
		fprintf(stderr, "Could not allocate video codec context\n");
		exit(1);
	}
	c->bit_rate = 4000000;
	c->width = width;
	c->height = height;
	c->time_base.num = 1;
	c->time_base.den = fps;
	c->gop_size = 10;
	c->max_b_frames = 1;
	c->pix_fmt = AV_PIX_FMT_YUV420P;
	if (codec_id == AV_CODEC_ID_H264)
		av_opt_set(c->priv_data, "preset", "slow", 0);
	if (avcodec_open2(c, codec, NULL) < 0) {
		fprintf(stderr, "Could not open codec\n");
		exit(1);
	}
	file = fopen(filename, "wb");
	if (!file) {
		fprintf(stderr, "Could not open %s\n", filename);
		exit(1);
	}
	frame = av_frame_alloc();
	if (!frame) {
		fprintf(stderr, "Could not allocate video frame\n");
		exit(1);
	}
	frame->format = c->pix_fmt;
	frame->width = c->width;
	frame->height = c->height;
	ret = av_image_alloc(frame->data, frame->linesize, c->width, c->height, c->pix_fmt, 32);
	if (ret < 0) {
		fprintf(stderr, "Could not allocate raw picture buffer\n");
		exit(1);
	}
}

void ffmpeg_encoder_finish(void) {
	uint8_t endcode[] = { 0, 0, 1, 0xb7 };
	int got_output, ret;
	do {
		fflush(stdout);
		ret = avcodec_encode_video2(c, &pkt, NULL, &got_output);
		if (ret < 0) {
			fprintf(stderr, "Error encoding frame\n");
			exit(1);
		}
		if (got_output) {
			fwrite(pkt.data, 1, pkt.size, file);
			av_packet_unref(&pkt);
		}
	} while (got_output);
	fwrite(endcode, 1, sizeof(endcode), file);
	fclose(file);
	avcodec_close(c);
	av_free(c);
	av_freep(&frame->data[0]);
	av_frame_free(&frame);
}

void ffmpeg_encoder_encode_frame(uint8_t* rgb) {
	int ret, got_output;
	ffmpeg_encoder_set_frame_yuv_from_rgb(rgb);
	av_init_packet(&pkt);
	pkt.data = NULL;
	pkt.size = 0;
	ret = avcodec_encode_video2(c, &pkt, frame, &got_output);
	if (ret < 0) {
		fprintf(stderr, "Error encoding frame\n");
		exit(1);
	}
	if (got_output) {
		fwrite(pkt.data, 1, pkt.size, file);
		av_packet_unref(&pkt);
	}
}

void ffmpeg_encoder_glread_rgb(uint8_t** rgb, GLubyte** pixels, unsigned int width, unsigned int height) {
	size_t i, j, k, cur_gl, cur_rgb, nvals;
	const size_t format_nchannels = 4;
	nvals = format_nchannels * width * height;
	*pixels = (GLubyte*)realloc(*pixels, nvals * sizeof(GLubyte));
	*rgb = (GLubyte*)realloc(*rgb, nvals * sizeof(uint8_t));
	/* Get RGBA to align to 32 bits instead of just 24 for RGB. May be faster for FFmpeg. */
	glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, *pixels);
	for (i = 0; i < height; i++) {
		for (j = 0; j < width; j++) {
			cur_gl = format_nchannels * (width * (height - i - 1) + j);
			cur_rgb = format_nchannels * (width * i + j);
			for (k = 0; k < format_nchannels; k++)
				(*rgb)[cur_rgb + k] = (*pixels)[cur_gl + k];
		}
	}
}

static void init(void) {
	int glget;

	if (offscreen) {
		/*  Framebuffer */
		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);

		/* Color renderbuffer. */
		glGenRenderbuffers(1, &rbo_color);
		glBindRenderbuffer(GL_RENDERBUFFER, rbo_color);
		/* Storage must be one of: */
		/* GL_RGBA4, GL_RGB565, GL_RGB5_A1, GL_DEPTH_COMPONENT16, GL_STENCIL_INDEX8. */
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB565, WIDTH, HEIGHT);
		glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo_color);

		/* Depth renderbuffer. */
		glGenRenderbuffers(1, &rbo_depth);
		glBindRenderbuffer(GL_RENDERBUFFER, rbo_depth);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, WIDTH, HEIGHT);
		glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo_depth);

		glReadBuffer(GL_COLOR_ATTACHMENT0);

		/* Sanity check. */
		assert(glCheckFramebufferStatus(GL_FRAMEBUFFER));
		glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &glget);
		assert(WIDTH * HEIGHT < (unsigned int)glget);
	}
	else {
		glReadBuffer(GL_BACK);
	}

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glEnable(GL_DEPTH_TEST);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glViewport(0, 0, WIDTH, HEIGHT);

	time0 = glfwGetTime();
	ffmpeg_encoder_start("result.mpg", AV_CODEC_ID_MPEG1VIDEO, 60, WIDTH, HEIGHT);
}

static void deinit(void) {
	printf("FPS = %f\n", 1000.0 * nframes / (double)(glfwGetTime() - time0));
	free(pixels);
	ffmpeg_encoder_finish();
	free(rgb);
	if (offscreen) {
		glDeleteFramebuffers(1, &fbo);
		glDeleteRenderbuffers(1, &rbo_color);
		glDeleteRenderbuffers(1, &rbo_depth);
	}
}

void processInput(GLFWwindow *window);


// camera
Camera camera(glm::vec3(4.0f, 4.0f, 20.0f));


// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// lighting
glm::vec3 lightPos(5.0f, 3.0f, 6.0f);

// force use advanced GPU (nvidia)
//extern "C" {
//	_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
//}

void paraboloidConfigurator(Paraboloid & myParaboloid);

void cylinderConfigurator(Cylinder & myCylinder);

void curvedBladeConfigurator(CurvedBlade & myCurvedBlade);

int main(int, char**)
{
	// GLFW init
	
	GLFWwindow * window = setupGLFW("MyWindow", WIDTH, HEIGHT);

	if (window == nullptr)
	{
		std::cout << "Failed to setup GLFW";
		return 1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	std::ifstream generationParametersFile;
	generationParametersFile.open("parameters.json");
	nlohmann::json parametersJsonObject;
	generationParametersFile >> parametersJsonObject;

	auto cameraDriver = CameraDriver(&camera);
	for (auto & routePoint : parametersJsonObject["route"]) {
		cameraDriver.addRoutePoint({
			{
				static_cast<float>(atof(routePoint["posX"].get<std::string>().c_str())),
				static_cast<float>(atof(routePoint["posY"].get<std::string>().c_str())),
				static_cast<float>(atof(routePoint["posZ"].get<std::string>().c_str()))
			},
			{
				static_cast<float>(atof(routePoint["directionX"].get<std::string>().c_str())),
				static_cast<float>(atof(routePoint["directionY"].get<std::string>().c_str())),
				static_cast<float>(atof(routePoint["directionZ"].get<std::string>().c_str()))
			},
			static_cast<float>(atof(routePoint["duration"].get<std::string>().c_str()))
		});	
	}
	
	
	// IMGUI init
	if (!setupImgui(window))
	{
		std::cout << "Failed to setup ImGUI";
		return 1;
	}

	init();
	
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    bool show_demo_window = false;
    bool show_start_window = true;
    ImVec4 clear_color = ImVec4(0.103f, 0.103f, 0.103f, 1.00f);
	ImVec4 draw_color = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);


	Shader lampShader("shaders/lampShader.vert", "shaders/lampShader.frag");
	Shader lightingShader("shaders/lightingShader.vert", "shaders/lightingShader.frag");

	// set up vertex data (and buffer(s)) and configure vertex attributes
	// ------------------------------------------------------------------
	float vertices[] = {
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		 0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

		 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
	};

	Paraboloid myParaboloid(0, 0, 10, 42, 42);
	Cylinder myCylinder(1, 1, 42);
	CurvedBlade myCurvedBlade(
		glm::vec2(-6.5f, -11.2f),
		glm::vec2(-1.0f, 1.0f),
		glm::vec2(1.0f, 0.01f),
		glm::vec2(-3.8f, 3.5f),
		10.0f,
		10
	);
	// first, configure the cube's VAO (and VBO)

	unsigned int cubeVBO, cubeVAO;

	glGenBuffers(1, &cubeVBO);
	glGenVertexArrays(1, &cubeVAO);

	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindVertexArray(cubeVAO);

	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);


	
	// second, configure the light's VAO (VBO stays the same; the vertices are the same for the light object which is also a 3D cube)
	unsigned int lightVAO;
	glGenVertexArrays(1, &lightVAO);
	glBindVertexArray(lightVAO);

	// we only need to bind to the VBO (to link it with glVertexAttribPointer), no need to fill it; the VBO's data already contains all we need (it's already bound, but we do it again for educational purposes)
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// std::cout << "Maximum nr of vertex attributes supported: " << nrAttributes << std::endl;


	Color myColor(glm::vec3(1.0f, 0.0f, 0.0f));
	bool isColorAnimate = false;
	bool isWireMode = false;
	bool isAxisVisible = true;
	bool isOrtho = false;
	bool isMSAA = false;

	int lampSpeed = 0;
	float lampRadius = 2;
	float lampHeight = 3.0f;
	float lampX = 5.0;
	float lampZ = 6.0;
	double angle = 30;

	float angleX = 0.0f;
	float angleY = 0.0f;
	float angleZ = 0.0f;

	float translateX = 4.0f;
	float translateY = 2.0f;
	float translateZ = 4.0f;

	float scaleX = 1.0f;
	float scaleY = 1.0f;
	float scaleZ = 1.0f;

	int fanSpeed = 10;
	int bladesNum = 16;

	glm::vec3 materialAmbient = glm::vec3(0.1, 0.1, 0.1);
	glm::vec3 materialDiffuse = glm::vec3(1.0, 1.0, 1.0);
	glm::vec3 materialSpecular = glm::vec3(0.5, 0.5, 0.5);
	float materialShininess = 64.0f;
    // Main loop
	
    while (!glfwWindowShouldClose(window))
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

		GLenum error = glGetError();

		if (error != GL_NO_ERROR)
		{
			std::cout << "ERROR WTF " << error << std::endl;
		}

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
		
        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);
        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.

		glm::vec3 currColor = myColor.getColor();

        if(show_start_window)
        {
			draw_color = ImVec4(currColor.x, currColor.y, currColor.z, 0.00f);

            ImGui::Begin("OpenGL Lab Control");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
                                                                    // Edit bools storing our window open/close state
            //ImGui::Checkbox("Another Window", &show_another_window);
			ImGui::Checkbox("Wire mode", &isWireMode);
			ImGui::Checkbox("Show axes", &isAxisVisible);
			ImGui::Checkbox("Use orthographic", &isOrtho);
			ImGui::Checkbox("Enable MSAA", &isMSAA);

			if (ImGui::CollapsingHeader("Fan settings"))
			{
				ImGui::SliderInt("Fan speed", &fanSpeed, 0, 69);
				ImGui::SliderInt("Blades number", &bladesNum, 0, 42);

				if (ImGui::TreeNode("Rotate fan##2"))
				{
					ImGui::SliderFloat("X angle", &angleX, 0.0f, 360.0f);
					ImGui::SliderFloat("Y angle", &angleY, 0.0f, 360.0f);
					ImGui::SliderFloat("Z angle", &angleZ, 0.0f, 360.0f);
					ImGui::TreePop();
				}

				if (ImGui::TreeNode("Scale fan##2"))
				{
					ImGui::SliderFloat("X scale", &scaleX, 0.01f, 10.0f);
					ImGui::SliderFloat("Y scale", &scaleY, 0.01f, 10.0f);
					ImGui::SliderFloat("Z scale", &scaleZ, 0.01f, 10.0f);
					ImGui::TreePop();
				}

				if (ImGui::TreeNode("Translate fan##2"))
				{
					ImGui::SliderFloat("X translate", &translateX, -10.0f, 10.0f);
					ImGui::SliderFloat("Y translate", &translateY, -10.0f, 10.0f);
					ImGui::SliderFloat("Z translate", &translateZ, -10.0f, 10.0f);
					ImGui::TreePop();
				}
			}

			if (ImGui::CollapsingHeader("Lamp settings"))
			{
				ImGui::SliderInt("Lamp speed", &lampSpeed, 0, 100);
				ImGui::SliderFloat("Lamp radius", &lampRadius, 0.1f, 9.99f);
				ImGui::SliderFloat("Lamp X pos", &lampX, -10.0f, 10.0f);
				ImGui::SliderFloat("Lamp Y pos", &lampHeight, -10.0f, 10.0f);
				ImGui::SliderFloat("Lamp Z pos", &lampZ, -10.0f, 10.0f);
				lightPos.x = lampX;
				lightPos.y = lampHeight;
				lightPos.z = lampZ;

			}

			if (ImGui::CollapsingHeader("Primitives settings"))
			{
				if (ImGui::TreeNode("Paraboloid config##2"))
				{
					paraboloidConfigurator(myParaboloid);
					ImGui::TreePop();
				}

				if (ImGui::TreeNode("Cylinder config##2"))
				{
					cylinderConfigurator(myCylinder);
					ImGui::TreePop();
				}

				if (ImGui::TreeNode("Curved blade config##2"))
				{
					curvedBladeConfigurator(myCurvedBlade);
					ImGui::TreePop();
				}

			}


			if (ImGui::CollapsingHeader("Colors & Shaders"))
			{
				ImGui::Checkbox("Do color animate", &isColorAnimate);
				myColor.doColorShift(isColorAnimate);
				ImGui::ColorEdit3("Background color", (float*)&clear_color);
				ImGui::ColorEdit3("Draw color", (float*)&draw_color); // Edit 3 floats representing a color

				if (ImGui::TreeNode("Material##2"))
				{
					ImGui::ColorEdit3("Ambient", (float*)&materialAmbient);
					ImGui::ColorEdit3("Diffuse", (float*)&materialDiffuse);
					ImGui::ColorEdit3("Specular", (float*)&materialSpecular);
					ImGui::SliderFloat("Shininess", &materialShininess, 0, 1024, "%.0f");
					ImGui::TreePop();
				}
			}


            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::Text("Camera position: X: %.3f | Y: %.3f | Z: %.3f", camera.Position.x, camera.Position.y, camera.Position.z);
            ImGui::Text("Camera direction: X: %.3f | Y: %.3f | Z: %.3f", camera.Front.x, camera.Front.y, camera.Front.z);
            ImGui::End();
        }

		if ((draw_color.x != currColor.x) || (draw_color.y != currColor.y) || (draw_color.z != currColor.z))
		{
			currColor.x = draw_color.x;
			currColor.y = draw_color.y;
			currColor.z = draw_color.z;
			myColor.setColor(currColor);
		}

		if (isMSAA)
		{
			glEnable(GL_MULTISAMPLE);
		}
		else
		{
			glDisable(GL_MULTISAMPLE);
		}

        // Rendering
        ImGui::Render();
		


		// per-frame time logic
		// --------------------
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;

		cameraDriver.update(deltaTime);
    	
		lastFrame = currentFrame;

		// input
		// -----
		processInput(window);

		// render
		// ------
		glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// be sure to activate shader when setting uniforms/drawing objects

		lightingShader.use();
		lightingShader.setVec3("objectColor", currColor);
		lightingShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
		lightingShader.setVec3("lightWorldPos", lightPos);
		lightingShader.setVec3("material.ambient", materialAmbient);
		lightingShader.setVec3("material.diffuse", materialDiffuse);
		lightingShader.setVec3("material.specular", materialSpecular);
		lightingShader.setFloat("material.shininess", materialShininess);
		//lightingShader.setVec3("viewPos", camera.Position);

		// view/projection transformations
		glm::mat4 projection;
		if (isOrtho)
		{
			projection = glm::ortho(-(float)getFramebufferWidth()/100, (float)getFramebufferWidth()/100, -(float)getFramebufferHeight()/100, (float)getFramebufferHeight()/100, 0.01f, 100.0f);
		}
		else
		{
			projection = glm::perspective(glm::radians(camera.Zoom), (float)getFramebufferWidth() / (float)getFramebufferHeight(), 0.1f, 100.0f);
		}
		glm::mat4 view = camera.GetViewMatrix();
		lightingShader.setMat4("projection", projection);
		lightingShader.setMat4("view", view);

		// world transformation
		glm::mat4 model = glm::mat4(1.0f);
		lightingShader.setMat4("model", model);

		if(isWireMode)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		// render the cube


		// start drawing fan
		// first calculate fan object transform matrix
		glm::mat4 fanBaseModel = glm::mat4(1.0f);
		fanBaseModel = glm::translate(fanBaseModel, glm::vec3(translateX, translateY, translateZ));
		fanBaseModel = glm::rotate(fanBaseModel, glm::radians(angleX), glm::vec3(1, 0, 0));
		fanBaseModel = glm::rotate(fanBaseModel, glm::radians(angleY), glm::vec3(0, 1, 0));
		fanBaseModel = glm::rotate(fanBaseModel, glm::radians(angleZ), glm::vec3(0, 0, 1));
		fanBaseModel = glm::scale(fanBaseModel, glm::vec3(scaleX, scaleY, scaleZ));

		// then start transform and draw primitive figures
		model = fanBaseModel;
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1, 0, 0));
		model = glm::scale(model, glm::vec3(0.2, 0.2, 3));
		lightingShader.setMat4("model", model);
		myCylinder.draw();

		model = fanBaseModel;
		model = glm::translate(model, glm::vec3(0, 3, 0));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1, 0, 0));
		model = glm::scale(model, glm::vec3(0.15, 0.15, 3));
		lightingShader.setMat4("model", model);
		myCylinder.draw();

		model = fanBaseModel;
		model = glm::translate(model, glm::vec3(0, -1.75, 0));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1, 0, 0));
		model = glm::scale(model, glm::vec3(0.3, 0.3, 0.5));
		lightingShader.setMat4("model", model);
		myCylinder.draw();

		model = fanBaseModel;
		model = glm::translate(model, glm::vec3(0, 4.5, 1.85));
		model = glm::scale(model, glm::vec3(0.65, 0.65, 0.65));
		lightingShader.setMat4("model", model);
		myCylinder.draw();


		float time = glfwGetTime();
		for (size_t i = 0; i < bladesNum; i++)
		{
			float angle = 360.0f / bladesNum * i + time * 10 * fanSpeed;
			model = fanBaseModel;
			model = glm::translate(model, glm::vec3(0, 4.5, 1.85));
			model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1, 0, 0));
			model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 1, 0));
			model = glm::translate(model, glm::vec3(1.25, 0, 0));
			model = glm::scale(model, glm::vec3(0.6, 0.325, 0.2));
			lightingShader.setMat4("model", model);
			myCurvedBlade.draw();
		}

		model = fanBaseModel;
		model = glm::translate(model, glm::vec3(0, 4.5, -0.475));
		model = glm::scale(model, glm::vec3(0.18, 0.18, 0.2));
		lightingShader.setMat4("model", model);
		myParaboloid.draw();

		model = fanBaseModel;
		model = glm::translate(model, glm::vec3(0, -1.915, 0));
		model = glm::scale(model, glm::vec3(0.18, 0.18, 4));
		lightingShader.setMat4("model", model);

		glBindVertexArray(cubeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		model = fanBaseModel;
		model = glm::translate(model, glm::vec3(0, -1.915, 0));
		model = glm::scale(model, glm::vec3(4, 0.18, 0.18));
		lightingShader.setMat4("model", model);

		glBindVertexArray(cubeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);



		// fan drawing end

		// also draw the lamp object

		angle += lampSpeed/10.0f;
		if (angle >= 360)
			angle = 0;
		if (lampSpeed)
		{
			lightPos.x = glm::cos(glm::radians(angle)) * lampRadius;
			lightPos.z = glm::sin(glm::radians(angle)) * lampRadius;
		}

		lampShader.use();
		lampShader.setMat4("projection", projection);
		lampShader.setMat4("view", view);
		lampShader.setVec3("objectColor", glm::vec3(1.0, 1.0, 1.0));

		model = glm::mat4(1.0f);
		model = glm::translate(model, lightPos);
		model = glm::scale(model, glm::vec3(0.2f)); // a smaller cube
		lampShader.setMat4("model", model);

		glBindVertexArray(lightVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		if (isAxisVisible)
		{
			// draw axis
			// X axis
			model = glm::mat4(1.0f);
			model = glm::scale(model, glm::vec3(1000, 0.10, 0.10));
			lampShader.setVec3("objectColor", glm::vec3(1.0, 0.0, 0.0));
			lampShader.setMat4("model", model);

			glBindVertexArray(cubeVAO);
			glDrawArrays(GL_TRIANGLES, 0, 36);

			// Y axis
			model = glm::mat4(1.0f);
			model = glm::scale(model, glm::vec3(0.10, 1000, 0.10));
			lampShader.setVec3("objectColor", glm::vec3(0.0, 1.0, 0.0));
			lampShader.setMat4("model", model);

			glBindVertexArray(cubeVAO);
			glDrawArrays(GL_TRIANGLES, 0, 36);

			// Z axis
			model = glm::mat4(1.0f);
			model = glm::scale(model, glm::vec3(0.10, 0.10, 1000));
			lampShader.setVec3("objectColor", glm::vec3(0.0, 0.0, 1.0));
			lampShader.setMat4("model", model);

			glBindVertexArray(cubeVAO);
			glDrawArrays(GL_TRIANGLES, 0, 36);

		}

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);

		frame->pts = nframes;
		ffmpeg_encoder_glread_rgb(&rgb, &pixels, WIDTH, HEIGHT);
		ffmpeg_encoder_encode_frame(rgb);
		nframes++;
    }

	deinit();
	
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS)
	{
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		glfwSetCursorPosCallback(window, nullptr);
	}
	if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS)
	{
		setFirstMouse();
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		glfwSetCursorPosCallback(window, mouse_callback);
	}

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
}

void paraboloidConfigurator(Paraboloid & myParaboloid)
{
	static int newParabLevel = 42;
	static int currParabLevel = 43;

	static int newParabEdges = 42;
	static int currParabEdges = 43;

	static float newParabHeight = 10.0f;
	static float currParabHeight = 4.1f;

	ImGui::SliderInt("Levels number", &newParabLevel, 2, 360);

	if (currParabLevel != newParabLevel)
	{
		currParabLevel = newParabLevel;
		myParaboloid.recalculate(0, 0, currParabHeight, currParabEdges, currParabLevel);
	}

	ImGui::SliderInt("P_Edges number", &newParabEdges, 3, 360);

	if (currParabEdges != newParabEdges)
	{
		currParabEdges = newParabEdges;
		myParaboloid.recalculate(0, 0, currParabHeight, currParabEdges, currParabLevel);
	}

	ImGui::SliderFloat("Height", &newParabHeight, 0.1, 100);

	if (currParabHeight != newParabHeight)
	{
		currParabHeight = newParabHeight;
		myParaboloid.recalculate(0, 0, currParabHeight, currParabEdges, currParabLevel);

	}
}

void cylinderConfigurator(Cylinder & myCylinder)
{
	static float newRadius = 1;
	static float currRadius = 2;

	static float newHeight = 1;
	static float currHeight = 2;

	static int newEdgesNum = 10;
	static int currEdgesNum = 11;

	ImGui::SliderInt("C_Edges number", &newEdgesNum, 3, 360);

	if (currEdgesNum != newEdgesNum)
	{
		currEdgesNum = newEdgesNum;
		myCylinder.recalculate(currRadius, currHeight, currEdgesNum);
	}

	ImGui::SliderFloat("C_Radius", &newRadius, 0.01, 10);

	if (currRadius != newRadius)
	{
		currRadius = newRadius;
		myCylinder.recalculate(currRadius, currHeight, currEdgesNum);
	}

	ImGui::SliderFloat("C_Height", &newHeight, 0.01, 10);

	if (currHeight != newHeight)
	{
		currHeight = newHeight;
		myCylinder.recalculate(currRadius, currHeight, currEdgesNum);
	}
}

void curvedBladeConfigurator(CurvedBlade & myCurvedBlade)
{

	static float p1_newHeight  = 1.0f;
	static float p1_currHeight = 2.0f;

	static float p2_newHeight = 2.0f;
	static float p2_currHeight = 1.0f;

	static float f_newControlPointX = -5.0f;
	static float f_currControlPointX = 2.0f;

	static float f_newControlPointY = 2.0f;
	static float f_currControlPointY = 1.0f;

	static float s_newControlPointX = 5.0f;
	static float s_currControlPointX = 2;

	static float s_newControlPointY = 1.0f;
	static float s_currControlPointY = 2;

	static float newSpinPerLevel = 0.5f;
	static float currSpinPerLevel = 2;

	static int newLevelsNum = 100;
	static int currLevelsNum = 11;

	ImGui::SliderFloat("Start height", &p1_newHeight, 0.001, 10);
	ImGui::SliderFloat("End height", &p2_newHeight, 0.001, 10);
	ImGui::SliderFloat("First control point X", &f_newControlPointX, -30, 30);
	ImGui::SliderFloat("First control point Y", &f_newControlPointY, -30, 30);
	ImGui::SliderFloat("Second control point X", &s_newControlPointX, -30, 30);
	ImGui::SliderFloat("Second control point Y", &s_newControlPointY, -30, 30);
	ImGui::SliderFloat("Spin per level", &newSpinPerLevel, 0, 10);
	ImGui::SliderInt("Levels number", &newLevelsNum, 0.001, 100);

	if (p1_currHeight != p1_newHeight)
	{
		p1_currHeight = p1_newHeight;
		myCurvedBlade.recalculate(
			glm::vec2(f_currControlPointX, f_currControlPointY),
			glm::vec2(-1.0f, p1_currHeight),
			glm::vec2(1.0f,  p2_currHeight),
			glm::vec2(s_currControlPointX, s_currControlPointY),
			currSpinPerLevel,
			currLevelsNum
		);
	}

	if (p2_currHeight != p2_newHeight)
	{
		p2_currHeight = p2_newHeight;
		myCurvedBlade.recalculate(
			glm::vec2(f_currControlPointX, f_currControlPointY),
			glm::vec2(-1.0f, p1_currHeight),
			glm::vec2(1.0f, p2_currHeight),
			glm::vec2(s_currControlPointX, s_currControlPointY),
			currSpinPerLevel,
			currLevelsNum
		);
	}

	if (f_currControlPointX != f_newControlPointX)
	{
		f_currControlPointX = f_newControlPointX;
		myCurvedBlade.recalculate(
			glm::vec2(f_currControlPointX, f_currControlPointY),
			glm::vec2(-1.0f, p1_currHeight),
			glm::vec2(1.0f, p2_currHeight),
			glm::vec2(s_currControlPointX, s_currControlPointY),
			currSpinPerLevel,
			currLevelsNum
		);
	}

	if (f_currControlPointY != f_newControlPointY)
	{
		f_currControlPointY = f_newControlPointY;
		myCurvedBlade.recalculate(
			glm::vec2(f_currControlPointX, f_currControlPointY),
			glm::vec2(-1.0f, p1_currHeight),
			glm::vec2(1.0f, p2_currHeight),
			glm::vec2(s_currControlPointX, s_currControlPointY),
			currSpinPerLevel,
			currLevelsNum
		);
	}

	if (s_currControlPointX != s_newControlPointX)
	{
		s_currControlPointX = s_newControlPointX;
		myCurvedBlade.recalculate(
			glm::vec2(f_currControlPointX, f_currControlPointY),
			glm::vec2(-1.0f, p1_currHeight),
			glm::vec2(1.0f, p2_currHeight),
			glm::vec2(s_currControlPointX, s_currControlPointY),
			currSpinPerLevel,
			currLevelsNum
		);
	}

	if (s_currControlPointY != s_newControlPointY)
	{
		s_currControlPointY = s_newControlPointY;
		myCurvedBlade.recalculate(
			glm::vec2(f_currControlPointX, f_currControlPointY),
			glm::vec2(-1.0f, p1_currHeight),
			glm::vec2(1.0f, p2_currHeight),
			glm::vec2(s_currControlPointX, s_currControlPointY),
			currSpinPerLevel,
			currLevelsNum
		);
	}

	if (currSpinPerLevel != newSpinPerLevel)
	{
		currSpinPerLevel = newSpinPerLevel;
		myCurvedBlade.recalculate(
			glm::vec2(f_currControlPointX, f_currControlPointY),
			glm::vec2(-1.0f, p1_currHeight),
			glm::vec2(1.0f, p2_currHeight),
			glm::vec2(s_currControlPointX, s_currControlPointY),
			currSpinPerLevel,
			currLevelsNum
		);
	}

	if (currLevelsNum != newLevelsNum)
	{
		currLevelsNum = newLevelsNum;
		myCurvedBlade.recalculate(
			glm::vec2(f_currControlPointX, f_currControlPointY),
			glm::vec2(-1.0f, p1_currHeight),
			glm::vec2(1.0f, p2_currHeight),
			glm::vec2(s_currControlPointX, s_currControlPointY),
			currSpinPerLevel,
			currLevelsNum
		);
	}

}










