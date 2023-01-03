// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <iostream>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
using namespace glm;

#include <common/shader.hpp>

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

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow( 1024, 768, "FPGA 0001", NULL, NULL);
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

	// Dark blue background
	glClearColor(0.5f, 0.5f, 0.5f, 0.5f);

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);



	// Create and compile our GLSL program from the shaders
	GLuint programID = LoadShaders( "SimpleVertexShader.vertexshader", "SimpleFragmentShader.fragmentshader" );

	// dimensions between -1 and +1
	float total_height = 1.8;
	float total_width = 2.0;
	float pad_height = total_height/4;
	float pad_width = total_width/5;
	float h_margins = pad_width*0.1;
	float v_margins = pad_height*0.1;

	// Vector of vertices (20 channels) * (2 triangles) * (3 vertices)
	std::vector<float> box1;//(1*3*1,0.0);
	std::vector<std::vector<float>> square_lines;

	for(int i=0; i<20; i++)
	{
		// 4 vertices A B C D (counter-clock wise from top left)
		// some redundant
		float x_A = (i%5)*pad_width + h_margins -total_width/2.0;
		float y_A = -((int)(i/5))*pad_height - v_margins +total_height/2.0;
		float x_B = x_A;
		float y_B = y_A - pad_height + 2*v_margins;
		float x_C = x_A + pad_width -2*h_margins;
		float y_C = y_B;
		float x_D = x_C;
		float y_D = y_A;

/*		std::cout << "i:" << i << "	x_A: " << x_A << "	y_A: " << y_A
													 << "	x_B: " << x_B << "	y_B: " << y_B
													 << "	x_C: " << x_C << "	y_C: " << y_C  << std::endl;
*/
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

		std::vector<float> square{x_A+h_margins, y_A+v_margins, 0.0,
															x_B+h_margins, y_B+v_margins, 0.0,
															x_C+h_margins, y_C+v_margins, 0.0,
															x_D+h_margins, y_D+v_margins, 0.0	};

	  square_lines.push_back(square);
	}

	static const GLfloat g_color_buffer_data[] = {.7f, .7f, .7f};

	std::cout << "box1.size(): " << box1.size()<< std::endl;

	GLuint vertexbuffer;
	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, box1.size() * sizeof(float), box1.data(), GL_STATIC_DRAW);

	/*GLuint VertexArrayID1;
	glGenVertexArrays(1, &VertexArrayID1);
	glBindVertexArray(VertexArrayID1);

	GLuint vertexbuffer1;
	glGenBuffers(1, &vertexbuffer1);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer1);
	glBufferData(GL_ARRAY_BUFFER, square_lines.size() * sizeof(float), square_lines.data(), GL_STATIC_DRAW);
*/
/*	GLuint colorbuffer;
	glGenBuffers(1, &colorbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_color_buffer_data), g_color_buffer_data, GL_STATIC_DRAW);
*/

	do{

		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use our shader
		glUseProgram(programID);

		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(
			0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		// 2nd attribute buffer : colors
	/*	glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
		glVertexAttribPointer(
			1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);
*/
		// Draw the triangle !
		glDrawArrays(GL_TRIANGLES, 0, box1.size()); // 3 indices starting at 0 -> 1 triangle

		// glEnableVertexAttribArray(0);
		// glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer1);
		// glVertexAttribPointer(
		// 	0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		// 	3,                  // size
		// 	GL_FLOAT,           // type
		// 	GL_FALSE,           // normalized?
		// 	0,                  // stride
		// 	(void*)0            // array buffer offset
		// );

		for(auto& it : square_lines)
		{
			glDrawArrays(GL_LINE_LOOP, 0, it.size()); // 3 indices starting at 0 -> 1 triangle
		}

		// glDrawArrays(GL_LINE_LOOP, 0, square_lines.size()); // 3 indices starting at 0 -> 1 triangle

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);


		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	}


	 // Check if the ESC key was pressed or the window was closed
	while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
		   glfwWindowShouldClose(window) == 0 );

	// Cleanup VBO
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteVertexArrays(1, &VertexArrayID);
	glDeleteProgram(programID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}
