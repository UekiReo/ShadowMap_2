#include <windows.h>

#include <GL/glew.h>

#include <GL/freeglut.h>

#include <GL/gl.h>
#include <GL/glext.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

#include "InitShader.h"
#include "LoadMesh.h"

static const std::string vertex_shader("shadowmap_vs.glsl");
static const std::string fragment_shader("shadowmap_fs.glsl");
GLuint shader_program = -1;

GLuint quad_vao = -1;
GLuint quad_vbo = -1;
bool change = false;
float w_num = 1.5;
bool enable = false;

GLuint fbo_id = -1;   
GLuint shadow_map_texture_id = -1;

int shadow_map_size = 1024; 

int win_width = 1024;
int win_height = 1024;

static const std::string mesh_name = "Amago0.obj";
MeshData mesh_data;

glm::mat4 V_cam = glm::lookAt(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
glm::mat4 P_cam = glm::perspective(100.0f, 1.0f, 0.1f, 30.0f);
//glm::mat4 V_light = glm::lookAt(glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
//glm::mat4 P_light = glm::perspective(60.0f, 1.0f, 0.1f, 30.0f);

glm::mat4 M_fish;
glm::mat4 M_quad = glm::translate(glm::vec3(0.0f, -0.8f, 0.0f))*glm::rotate(-20.0f, glm::vec3(0.0f, 0.0f, 0.0f))*glm::scale(glm::vec3(2.0f));

int render_mode = 1;

bool check_framebuffer_status();

void draw_scene(glm::mat4& V)
{
   int VM_loc = glGetUniformLocation(shader_program, "VM");
   if(VM_loc != -1)
   {
      const glm::mat4 VM = V*M_fish;
      glUniformMatrix4fv(VM_loc, 1, false, glm::value_ptr(VM));
   }
   glBindVertexArray(mesh_data.mVao);
	glDrawElements(GL_TRIANGLES, mesh_data.mNumIndices, GL_UNSIGNED_INT, 0);

   if(VM_loc != -1)
   {
      const glm::mat4 VM = V*M_quad;
      glUniformMatrix4fv(VM_loc, 1, false, glm::value_ptr(VM));
   }
   glBindVertexArray(quad_vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void draw_pass_1() 
{
   const int pass = 1;

   const int pass_loc = glGetUniformLocation(shader_program, "pass");
   if(pass_loc != -1)
   {
      glUniform1i(pass_loc, pass);
   }
  
   const int P_loc = glGetUniformLocation(shader_program, "P");
   if(P_loc != -1)
   {
      //glUniformMatrix4fv(P_loc, 1, false, glm::value_ptr(P_light));
   }

   glEnable(GL_POLYGON_OFFSET_FILL);
   glPolygonOffset(50.0, 30.0); 

   //draw_scene(V_light);
   glDisable(GL_POLYGON_OFFSET_FILL);
}

void draw_pass_2()
{
   const int pass = 2;

   const int pass_loc = glGetUniformLocation(shader_program, "pass");
   if(pass_loc != -1)
   {
      glUniform1i(pass_loc, pass);
   }
   const int w_loc = glGetUniformLocation(shader_program, "w_num");
   if (w_loc != -1)
   {
       glUniform1f(w_loc,w_num);
   }
   const int change_loc = glGetUniformLocation(shader_program, "change");
   if (change_loc != -1)
   {
       glUniform1i(change_loc, change);
   }
   const int enable_loc = glGetUniformLocation(shader_program, "enable");
   if (enable_loc != -1)
   {
       glUniform1i(enable_loc, enable);
   }
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, shadow_map_texture_id);

   const int tex_loc = glGetUniformLocation(shader_program, "shadowmap");
   if(tex_loc != -1)
   {
      glUniform1i(tex_loc, 0);
   }

   const int P_loc = glGetUniformLocation(shader_program, "P");
   if(P_loc != -1)
   {
      glUniformMatrix4fv(P_loc, 1, false, glm::value_ptr(P_cam));
   }

   const int Shadow_loc = glGetUniformLocation(shader_program, "Shadow");
   const glm::mat4 S = glm::translate(glm::vec3(0.5f)) * glm::scale(glm::vec3(0.5f));
   if(Shadow_loc != -1)
   {
      //const glm::mat4 Shadow = S*P_light*V_light*glm::inverse(V_cam); 
      //glUniformMatrix4fv(Shadow_loc, 1, false, glm::value_ptr(Shadow));
   }

   draw_scene(V_cam);
}


void display()
{
   glUseProgram(shader_program);

   if(render_mode == 1) 
   {
      glBindFramebuffer(GL_FRAMEBUFFER, fbo_id); 
      glDrawBuffer(GL_NONE);
      glViewport(0, 0, shadow_map_size, shadow_map_size); 
      glClear(GL_DEPTH_BUFFER_BIT);
      draw_pass_1();
   
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      glDrawBuffer(GL_BACK);
      glViewport(0, 0, win_width, win_height);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  
      draw_pass_2();
   }
   else if (render_mode == 2) 
   {
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      glDrawBuffer(GL_BACK);
      glViewport(0, 0, shadow_map_size, shadow_map_size);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  
      draw_pass_1();
   }
   
   glutSwapBuffers();
}

void idle()
{
	glutPostRedisplay();

   const int time_ms = glutGet(GLUT_ELAPSED_TIME);
   float time_sec = 0.001f*time_ms;

   //M_fish = glm::rotate(15.0f*time_sec, glm::vec3(1.0f, 0.0f, 0.0f))*glm::scale(glm::vec3(mesh_data.mScaleFactor));
   
}

void reload_shader()
{
   GLuint new_shader = InitShader(vertex_shader.c_str(), fragment_shader.c_str());

   if(new_shader == -1)
   {
      glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
   }
   else
   {
      glClearColor(0.35f, 0.35f, 0.35f, 0.0f);

      if(shader_program != -1)
      {
         glDeleteProgram(shader_program);
      }
      shader_program = new_shader;

      if(mesh_data.mVao != -1)
      {
         BufferIndexedVerts(mesh_data);
      }
   }
}


void reshape(int w, int h)
{
   win_width = w;
   win_height = h;
   glViewport(0, 0, w, h);
}

void change_pass1() {
 
    if (change == false) {
        change = true;
    }
    else if (change == true) {
        change = false;
    }
}

void change_softness() {
  
    if (w_num < 7) {
        w_num += 0.5;
    }
    else
        w_num = 1.5;
}
void change_lit() {
    if (enable == false) {
        enable = true;
    }
    else if (enable == true) {
        enable = false;
    }
}

void keyboard(unsigned char key, int x, int y)
{
   std::cout << "key : " << key << ", x: " << x << ", y: " << y << std::endl;

   switch(key)
   {   
      case 'a':
      case 'A':
        change_softness();
        break;
      case 'l':
      case'L':
          change_lit();
          break;
      case 'm':
      case 'M':
          change_pass1();
          break;

      case 'r':
      case 'R':
         reload_shader();     
      break;

      case '1':
      case '2':
         render_mode = key-'1'+1;     
      break;
   }
}

void printGlInfo()
{
   std::cout << "Vendor: "       << glGetString(GL_VENDOR)                    << std::endl;
   std::cout << "Renderer: "     << glGetString(GL_RENDERER)                  << std::endl;
   std::cout << "Version: "      << glGetString(GL_VERSION)                   << std::endl;
   std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION)  << std::endl;
}

void initOpenGl()
{
   glewInit();

   std::cout<<"Press 1 to show scene\n";
   std::cout<<"Press 2 to show shadow map texture\n";
   std::cout<<"Press r to reload shader\n";
   std::cout << "Press m to change between 2x2 PCF and 4x4 PCF\n";
   std::cout << "Press l to enable/disable displaying lit4 as a vec4 color \n";
   std::cout << "Press a to change softness of shadow (make sure it's in 4x4 PCF first)\n";


   glEnable(GL_DEPTH_TEST);

   reload_shader();

   mesh_data = LoadMesh(mesh_name);
   M_fish = glm::scale(glm::vec3(mesh_data.mScaleFactor));

   quad_vbo = -1;

   glGenVertexArrays(1, &quad_vao);
   glBindVertexArray(quad_vao);

   float vertices[] = {1.0f, 1.0f, 0.0f, 1.0f, -1.0f, 0.0f, -1.0f, 1.0f, 0.0f, -1.0f, -1.0f, 0.0f};

   glGenBuffers(1, &quad_vbo);
   glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
   glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
   int pos_loc = glGetAttribLocation(shader_program, "pos_attrib");
   if(pos_loc >= 0)
   {
      glEnableVertexAttribArray(pos_loc);
	   glVertexAttribPointer(pos_loc, 3, GL_FLOAT, false, 0, 0);
   }

   glGenTextures(1, &shadow_map_texture_id);
   glBindTexture(GL_TEXTURE_2D, shadow_map_texture_id);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, shadow_map_size, shadow_map_size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
   glBindTexture(GL_TEXTURE_2D, 0);   

   glGenFramebuffers(1, &fbo_id);
   glBindFramebuffer(GL_FRAMEBUFFER, fbo_id);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadow_map_texture_id, 0);
   check_framebuffer_status();
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


int main (int argc, char **argv)
{
   glutInit(&argc, argv); 
   glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
   glutInitWindowPosition (5, 5);
   glutInitWindowSize (win_width, win_height);
   int win = glutCreateWindow ("Shadow Map");

   printGlInfo();

   glutDisplayFunc(display); 
   glutKeyboardFunc(keyboard);
   glutIdleFunc(idle);
   glutReshapeFunc(reshape);

   initOpenGl();

   glutMainLoop();
   glutDestroyWindow(win);
   return 0;		
}

bool check_framebuffer_status() 
{
    GLenum status;
    status = (GLenum) glCheckFramebufferStatus(GL_FRAMEBUFFER);
    switch(status) {
        case GL_FRAMEBUFFER_COMPLETE:
            return true;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			printf("Framebuffer incomplete, incomplete attachment\n");
            return false;
        case GL_FRAMEBUFFER_UNSUPPORTED:
			printf("Unsupported framebuffer format\n");
            return false;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			printf("Framebuffer incomplete, missing attachment\n");
            return false;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
			printf("Framebuffer incomplete, missing draw buffer\n");
            return false;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
			printf("Framebuffer incomplete, missing read buffer\n");
            return false;
    }
	return false;
}


