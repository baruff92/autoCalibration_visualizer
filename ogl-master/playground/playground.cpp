// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <cmath>
#include <algorithm>
// #include <ft2build.h>
// #inlcude FT_FREETYPE_H

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// Include GLM
#include <external/glm-0.9.7.1/glm/glm.hpp>
#include <external/glm-0.9.7.1/glm/gtc/matrix_transform.hpp>
using namespace glm;

#include <common/shader.hpp>
#include <common/texture.hpp>
#include <common/controls.hpp>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>
#include <common/text2D.hpp>

int main( void )
{
	// Initialise GLFW
	if( !glfwInit() )
	{
		fprintf( stderr, "Failed to initialize GLFW\n" );
		getchar();
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


	// Open a window and create its OpenGL context
	int window_width = 1280;
	int window_height = 360;

	// vector containing the target rates
	// later to be filled with real values from caloChannels class
	// assuming the same 3 for every channel
	float target_diff_rate_th1 = 1.5; 	// Hz
	float target_diff_rate_th2 = 1.0; 	// Hz
	float target_diff_rate_th3 = 0.9; 	// Hz

	float rate_tolerance = 0.2; // Hz

	//	60 measured rates (ordered : CH0 thr1 thr2 thr3
	//															 CH1 thr1 thr2 thr3
	//																					 etc...)
	// now random generated
	// later from measurements
	std::vector<float> measured_rates;
  srand (time(NULL));
	for(int i=0; i< 60; i++)
	{
		float random_rate = (rand() % 300)/100.0 ;
		measured_rates.emplace_back(random_rate);
		std::cout << "[" << i << "]	" << measured_rates[i] << std::endl;
	}

	window = glfwCreateWindow( window_width, window_height, "FPGA 0001", NULL, NULL);
	if( window == NULL ){
		fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
		getchar();
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return -1;
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	// Set the mouse at the center of the screen
	glfwPollEvents();
	glfwSetCursorPos(window, 1024/2, 768/2);


	// Dark blue background
	glClearColor(0.5f, 0.5f, 0.5f, 0.5f);

	// Enable depth test
	// glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	// glDepthFunc(GL_LESS);

	// Create and compile our GLSL program from the shaders
	GLuint programID = LoadShaders( "SimpleVertexShader.vertexshader", "SimpleFragmentShader.fragmentshader" );
	// GLuint programID = LoadShaders( "../tutorial11_2d_fonts/StandardShading.vertexshader", "../tutorial11_2d_fonts/StandardShading.fragmentshader" );

	//*****************************************************************
	// for text
	// Get a handle for our "MVP" uniform
	// GLuint MatrixID = glGetUniformLocation(programID, "MVP");
	// GLuint ViewMatrixID = glGetUniformLocation(programID, "V");
	// GLuint ModelMatrixID = glGetUniformLocation(programID, "M");

	// Load the texture
	GLuint Texture = loadDDS("uvmap.DDS");

	// Get a handle for our "myTextureSampler" uniform
	GLuint TextureID = glGetUniformLocation(programID, "myTextureSampler");

	//*****************************************************************

	// y range of the histograms

	auto max = std::max_element(measured_rates.begin(), measured_rates.end());
	float rate_range = std::ceil(*max); //Hz
	std::cout << "Max measure rate: " << *max << " Hz	-	Y range: " << rate_range << " Hz" <<  std::endl;

	// dimensions between -1 and +1
	float total_height = 2.0;
	float total_width = 2.0;
	float pad_height = total_height/2;
	float pad_width = total_width/10;
	float h_margins = pad_width*0.12;
	float v_margins = pad_height*0.1;
	float v_offset = 0.0;


	// Vector of vertices (20 channels) * (2 triangles) * (3 vertices)
	std::vector<float> box1;
	std::vector<float> square_lines;
	std::vector<float> vertical_threshold_divisions;
	std::vector<float> target_rates_lines;
	std::vector<float> measured_rates_lines;
	std::vector<float> measured_rates_colors;

	std::vector<float> orange{255.0/255.0,165.0/255.0,0.0/255.0};
	std::vector<float> gold{255.0/255.0,215.0/255.0,0.0/255.0};
	std::vector<float> green{50.0/205.0,215.0/255.0,50.0/255.0};

	for(int i=0; i<20; i++)
	{
		// 4 vertices A B C D (counter-clock wise from top left)
		// some redundant
		float x_A = (i%10)*pad_width + h_margins -total_width/2.0;
		float y_A = -((int)(i/10))*pad_height - v_margins +total_height/2.0 - v_offset;
		float x_B = x_A;
		float y_B = y_A - pad_height + 2*v_margins - v_offset;
		float x_C = x_A + pad_width -2*h_margins;
		float y_C = y_B;
		float x_D = x_C;
		float y_D = y_A;

		// first triangle is ABC
		std::vector<float> first_triangle{x_A, y_A, 0.0,
																			x_B, y_B, 0.0,
																			x_C, y_C, 0.0	};

		box1.insert(box1.end(), std::begin(first_triangle), std::end(first_triangle));

		// second triangle is ACD
		std::vector<float> second_triangle{x_A, y_A, 0.0,
																			x_C, y_C, 0.0,
																			x_D, y_D, 0.0	};

		box1.insert(box1.end(), std::begin(second_triangle), std::end(second_triangle));

		std::vector<float> square{x_A, y_A, 0.0,
															x_B, y_B, 0.0,
															x_B, y_B, 0.0,
															x_C, y_C, 0.0,
															x_C, y_C, 0.0,
															x_D, y_D, 0.0,
															x_D, y_D, 0.0,
															x_A, y_A, 0.0	};

		square_lines.insert(square_lines.end(), std::begin(square), std::end(square));

		std::vector<float> vertical_dashed_th1;
		std::vector<float> vertical_dashed_th2;

		// coordiantes of the divisions between thresholds inside a channel pad
		float x_th1 = x_A + (x_C-x_A)*1/3.0;
		float x_th2 = x_A + (x_C-x_A)*2/3.0;
		float number_of_dashes = 15.;	// better a even number
		float number_of_divisions = 2*number_of_dashes;

		for(int j=0; j<number_of_divisions; j++)
		{
			float y_th = y_B+(y_A-y_B)*j/number_of_divisions;

			vertical_dashed_th1.emplace_back(x_th1);
			vertical_dashed_th1.emplace_back(y_th);
			vertical_dashed_th1.emplace_back(0.0);

			vertical_dashed_th2.emplace_back(x_th2);
			vertical_dashed_th2.emplace_back(y_th);
			vertical_dashed_th2.emplace_back(0.0);

			// std::cout << j << " y_B: " << y_B << " x: " << x_th1 << " y: " << y_B+(y_A-y_B)*j/number_of_divisions << "y_A: " << y_A << std::endl;
		}

		vertical_threshold_divisions.insert(vertical_threshold_divisions.end(), std::begin(vertical_dashed_th1), std::end(vertical_dashed_th1));
		vertical_threshold_divisions.insert(vertical_threshold_divisions.end(), std::begin(vertical_dashed_th2), std::end(vertical_dashed_th2));

		float y_target_rate_1 = (y_A-y_B)/rate_range*target_diff_rate_th1 + y_B;
		float y_target_rate_2 = (y_A-y_B)/rate_range*target_diff_rate_th2 + y_B;
		float y_target_rate_3 = (y_A-y_B)/rate_range*target_diff_rate_th3 + y_B;

		std::vector<float> target_rates_lines_temp{x_A,	y_target_rate_1, 0.0,
																							 x_th1, y_target_rate_1, 0.0,
																							 x_th1,	y_target_rate_2, 0.0,
 																							 x_th2, y_target_rate_2, 0.0,
																							 x_th2,	y_target_rate_3, 0.0,
 																							 x_C, y_target_rate_3, 0.0 };

		target_rates_lines.insert(target_rates_lines.end(),
															std::begin(target_rates_lines_temp), std::end(target_rates_lines_temp));

		// std::cout << "[" << i << "] " << measured_rates[i*3] << " " << measured_rates[i*3+1] << " " << measured_rates[i*3+2] << std::endl;

		float y_measured_rate_1 = (y_A-y_B)/rate_range*measured_rates[i*3] + y_B;
		float y_measured_rate_2 = (y_A-y_B)/rate_range*measured_rates[i*3+1] + y_B;
		float y_measured_rate_3 = (y_A-y_B)/rate_range*measured_rates[i*3+2] + y_B;

		std::vector<float> measured_rates_lines_temp_1{x_A,	y_measured_rate_1, 0.0,
																									 x_A,	y_B, 0.0,
																									 x_th1,	y_B, 0.0,
																									 x_A,	y_measured_rate_1, 0.0,
																									 x_th1,	y_B, 0.0,
																									 x_th1,	y_measured_rate_1, 0.0 };

		measured_rates_lines.insert(measured_rates_lines.end(),
																std::begin(measured_rates_lines_temp_1), std::end(measured_rates_lines_temp_1));

		if(measured_rates[i*3] < target_diff_rate_th1 - rate_tolerance)
		{
		for(int f=0; f<6; f++) measured_rates_colors.insert(measured_rates_colors.end(),
																std::begin(gold), std::end(gold));
		}

		if(measured_rates[i*3] > target_diff_rate_th1 + rate_tolerance)
		{
		for(int f=0; f<6; f++) measured_rates_colors.insert(measured_rates_colors.end(),
																std::begin(orange), std::end(orange));
		}

		if(measured_rates[i*3] >= target_diff_rate_th1 - rate_tolerance &&
				measured_rates[i*3] <= target_diff_rate_th1 + rate_tolerance)
		{
		for(int f=0; f<6; f++) measured_rates_colors.insert(measured_rates_colors.end(),
																std::begin(green), std::end(green));
		}

		std::vector<float> measured_rates_lines_temp_2{x_th1,	y_measured_rate_2, 0.0,
																									 x_th1,	y_B, 0.0,
																									 x_th2,	y_B, 0.0,
																									 x_th1,	y_measured_rate_2, 0.0,
																									 x_th2,	y_B, 0.0,
																									 x_th2,	y_measured_rate_2, 0.0 };

		measured_rates_lines.insert(measured_rates_lines.end(),
																std::begin(measured_rates_lines_temp_2), std::end(measured_rates_lines_temp_2));

		if(measured_rates[i*3+1] < target_diff_rate_th2 - rate_tolerance)
		{
		for(int f=0; f<6; f++) measured_rates_colors.insert(measured_rates_colors.end(),
																std::begin(gold), std::end(gold));
		}

		if(measured_rates[i*3+1] > target_diff_rate_th2 + rate_tolerance)
		{
		for(int f=0; f<6; f++) measured_rates_colors.insert(measured_rates_colors.end(),
																std::begin(orange), std::end(orange));
		}

		if(measured_rates[i*3+1] >= target_diff_rate_th2 - rate_tolerance &&
				measured_rates[i*3+1] <= target_diff_rate_th2 + rate_tolerance)
		{
		for(int f=0; f<6; f++) measured_rates_colors.insert(measured_rates_colors.end(),
																std::begin(green), std::end(green));
		}


		std::vector<float> measured_rates_lines_temp_3{x_th2,	y_measured_rate_3, 0.0,
																									 x_th2,	y_B, 0.0,
																									 x_C,	y_B, 0.0,
																									 x_th2,	y_measured_rate_3, 0.0,
																									 x_C,	y_B, 0.0,
																									 x_C,	y_measured_rate_3, 0.0 };

		measured_rates_lines.insert(measured_rates_lines.end(),
																std::begin(measured_rates_lines_temp_3), std::end(measured_rates_lines_temp_3));

		if(measured_rates[i*3+2] < target_diff_rate_th3 - rate_tolerance)
		{
		for(int f=0; f<6; f++) measured_rates_colors.insert(measured_rates_colors.end(),
																std::begin(gold), std::end(gold));
		}

		if(measured_rates[i*3+2] > target_diff_rate_th3 + rate_tolerance)
		{
		for(int f=0; f<6; f++) measured_rates_colors.insert(measured_rates_colors.end(),
																std::begin(orange), std::end(orange));
		}

		if(measured_rates[i*3+2] >= target_diff_rate_th3 - rate_tolerance &&
				measured_rates[i*3+2] <= target_diff_rate_th3 + rate_tolerance)
		{
		for(int f=0; f<6; f++) measured_rates_colors.insert(measured_rates_colors.end(),
																std::begin(green), std::end(green));
		}

	}


	// Colors of squares =  white
	int size_of_color = box1.size();
  std::vector<float> colors_vector(size_of_color, 0.0);

	GLuint m_vaoID[6];			// two vertex array objects, one for each drawn object
	GLuint m_vboID[7];			// three VBOs

	// Two VAOs allocation
	glGenVertexArrays(6, &m_vaoID[0]);

	glBindVertexArray(m_vaoID[5]);

	glGenBuffers(1, &m_vboID[6]);

	glBindBuffer(GL_ARRAY_BUFFER, m_vboID[6]);
	glBufferData(GL_ARRAY_BUFFER, box1.size() * sizeof(float), box1.data(), GL_STATIC_DRAW);
	glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

/*	glBindBuffer(GL_ARRAY_BUFFER, m_vboID[1]);
	glBufferData(GL_ARRAY_BUFFER, colors_vector.size() * sizeof(float), colors_vector.data(), GL_STATIC_DRAW);
	glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);
*/
	// Second VAO setup
	glBindVertexArray(m_vaoID[1]);
	glGenBuffers(1, &m_vboID[1]);
	glBindBuffer(GL_ARRAY_BUFFER, m_vboID[1]);
	glBufferData(GL_ARRAY_BUFFER, square_lines.size() * sizeof(float), square_lines.data(), GL_STATIC_DRAW);
	glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	// third VAO setup
	glBindVertexArray(m_vaoID[2]);
	glGenBuffers(1, &m_vboID[2]);
	glBindBuffer(GL_ARRAY_BUFFER, m_vboID[2]);
	glBufferData(GL_ARRAY_BUFFER, vertical_threshold_divisions.size() * sizeof(float), vertical_threshold_divisions.data(), GL_STATIC_DRAW);
	glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	// fourth VAO setup
	glBindVertexArray(m_vaoID[3]);
	glGenBuffers(1, &m_vboID[3]);
	glBindBuffer(GL_ARRAY_BUFFER, m_vboID[3]);
	glBufferData(GL_ARRAY_BUFFER, target_rates_lines.size() * sizeof(float), target_rates_lines.data(), GL_STATIC_DRAW);
	glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	// fifth VAO setup
	glBindVertexArray(m_vaoID[4]);
	glGenBuffers(1, &m_vboID[4]);
	glBindBuffer(GL_ARRAY_BUFFER, m_vboID[4]);
	glBufferData(GL_ARRAY_BUFFER, measured_rates_lines.size() * sizeof(float), measured_rates_lines.data(), GL_STATIC_DRAW);
	glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glEnableVertexAttribArray(1);
	glGenBuffers(1, &m_vboID[5]);
	glBindBuffer(GL_ARRAY_BUFFER, m_vboID[5]);
	glBufferData(GL_ARRAY_BUFFER, measured_rates_colors.size() * sizeof(float), measured_rates_colors.data(), GL_STATIC_DRAW);
	// glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// glBindVertexArray(0);

	std::cout << "m_vaoID[0]: " << m_vaoID[0] << "	m_vaoID[1]: " << m_vaoID[1] << "	m_vaoID[2]: " << m_vaoID[2] << std::endl;
	std::cout << "m_vboID[0]: " << m_vboID[0] << "	m_vboID[1]: " << m_vboID[1] << "	m_vboID[2]: " << m_vboID[2]  << std::endl;


	// Clear vectors
/*	colors_vector.clear();
	box1.clear();
	square_lines.clear();
*/
	// ??
	static const GLfloat g_color_buffer_data[] = {.7f, .7f, .7f};

	std::cout << "box1.size(): " << box1.size()<< std::endl;
	std::cout << "square_lines.size(): " << square_lines.size()<< std::endl;

	// int const w = glutGet(GLUT_WINDOW_WIDTH);
	// int const h = glutGet(GLUT_WINDOW_HEIGHT);

	// std::cout << "GLUT_WINDOW_WIDTH: " << w << "	GLUT_WINDOW_HEIGHT: " << h << std::endl;

	// Initialize our little text library with the Holstein font
	initText2D( "Holstein.DDS" );

	unsigned int parametric_position = 0;

	do{

		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use our shader
		glUseProgram(programID);

		glBindVertexArray(m_vaoID[5]);		// select first VAO
		glVertexAttrib3f((GLuint)1, 1.0, 1.0, 1.0); // set constant color attribute
		glDrawArrays(GL_TRIANGLES, 0, box1.size());	// draw first object


		glBindVertexArray(m_vaoID[4]);		// select fifth VAO
		glEnableVertexAttribArray(1);
		// 2nd attribute buffer : colors
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, m_vboID[5]);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0 );
		// glVertexAttrib3f((GLuint)1, 1.0, 1.0, 1.0); // set constant color attribute
		glDrawArrays(GL_TRIANGLES, 0, measured_rates_lines.size());	// draw fourth object

		glBindVertexArray(m_vaoID[3]);		// select fourth VAO
		glVertexAttrib3f((GLuint)1, 1.0, 0.0, 0.0); // set constant color attribute
		glDrawArrays(GL_LINES, 0, target_rates_lines.size());	// draw fourth object

		glBindVertexArray(m_vaoID[2]);		// select third VAO
		glVertexAttrib3f((GLuint)1, 0.0, 0.0, 1.0); // set constant color attribute
		glDrawArrays(GL_LINES, 0, vertical_threshold_divisions.size());	// draw third object

		glBindVertexArray(m_vaoID[1]);		// select second VAO
		glVertexAttrib3f((GLuint)1, 0.0, 0.0, 0.0); // set constant color attribute
		glDrawArrays(GL_LINES, 0, square_lines.size());	// draw second object

		glBindVertexArray(1);
		// Bind our texture in Texture Unit 0
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glEnable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture);
		// Set our "myTextureSampler" sampler to use Texture Unit 0
		glUniform1i(TextureID, 0);
		// glBindTexture(GL_TEXTURE_2D, Texture);

//		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
		glColor3d(0,1,0);
		// glVertexAttrib3f((GLuint)1, 1.0, 1.0, 1.0); // set constant color attribute

		 glDisableVertexAttribArray(0);
		// glDisableVertexAttribArray(1);

/*		parametric_position = parametric_position +5;

		double x_text = parametric_position % 500;
		double y_text = (int)(parametric_position/10)%600  ;
		char text[256];
		sprintf(text,"Porcanna troia");
*/

	//	printText2D(text, x_text, y_text, 60);

		// Write titles over graphs

		// dimensions between -1 and +1
/*		float total_height = 2.0;
		float total_width = 2.0;
		float pad_height = total_height/4;
		float pad_width = total_width/5;
		float h_margins = pad_width*0.05;
		float v_margins = pad_height*0.1;
		float v_offset = 0.0;

		float x_A = (i%5)*pad_width + h_margins -total_width/2.0;
		float y_A = -((int)(i/5))*pad_height - v_margins +total_height/2.0 - v_offset;
		float x_B = x_A;
		float y_B = y_A - pad_height + 2*v_margins - v_offset;
		float x_C = x_A + pad_width -2*h_margins;
		float y_C = y_B;
		float x_D = x_C;
		float y_D = y_A;
		*/

		float real_window_width = 800; // window_height = 900
		float real_window_height = 600; // window_height = 900
		int label_size = 15;

		// titles of pads and labels
		for(int i=0; i<20; i++ )
		{
			float title_x = real_window_width/20*((i%10)*2+0.5);
			float title_y = real_window_height - ((int)i/10*2 +0.2 )*real_window_height/4 ;
			char text[5];
			sprintf(text,"Ch%i", i);
			printText2D(text, title_x , title_y, 20);
			// std::cout << text << "	title_x: " << title_x << "	title_y: " << title_y << std::endl;

			float label_x = real_window_width/2 * ((i%10)*pad_width + h_margins -total_width/2.0) + real_window_width/2 -label_size*0.7;
			float label_y_high = real_window_height/2 * (-((int)(i/10))*pad_height - v_margins +total_height/2.0) + real_window_height/2;
			float label_y_low = real_window_height/2 * (-((int)(i/10))*pad_height - v_margins +total_height/2.0 - pad_height + 2*v_margins) + real_window_height/2;

			// std::cout << "[" << i << "]	label_x: " << label_x << "	label_y_low: " << label_y_low << "	label_y_high: " << label_y_high << std::endl;

			char text_0[5];
			sprintf(text_0,"0");
			printText2D(text_0, label_x , label_y_low, label_size);

			char text_1[50];
			sprintf(text_1,"%.0f", rate_range);
			printText2D(text_1, label_x , label_y_high, label_size);

			char text_2[50];
			sprintf(text_2,"Hz");
			printText2D(text_2, label_x - label_size*0.2, (label_y_high+label_y_low)/2, label_size);
		}

		glBindVertexArray(0);

		// Swap buffers

		glfwSwapBuffers(window);
		glfwPollEvents();

	}


	 // Check if the ESC key was pressed or the window was closed
	while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
		   glfwWindowShouldClose(window) == 0 );

	 // glDeleteVertexArrays(1, &VertexArrayID);
	 cleanupText2D();
	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}
