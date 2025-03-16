/**
 * MOON Engine - minimalist OpenGL open-source engine
 * lightweight and customizable game engine targeting older systems
 * designed for Windows 98/XP, Mac, and Linux compatibility
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <time.h>
 #include <math.h>
 
 // use older OpenGL for maximum compatibility
 #ifdef _WIN32
 #include <windows.h>
 #endif
 #include <GL/gl.h>
 
 // platform-specific includes
 #ifdef _WIN32
 #include <GL/glaux.h>
 #elif defined(__APPLE__)
 #include <OpenGL/gl.h>
 #include <GLUT/glut.h>
 #else
 #include <GL/glut.h>
 #endif
 
 // configuration constants
 #define WINDOW_WIDTH 320
 #define WINDOW_HEIGHT 240
 #define WINDOW_TITLE "MOON Engine v0.1"
 #define CONFIG_FILE "config.ini"
 #define MAP_FILE "map.txt"
 #define MAP_SIZE 32 // Maximum map dimensions
 
 // camera structure
 typedef struct {
     float pos_x, pos_y, pos_z;    // position
     float dir_x, dir_z;           // direction vector (x,z plane)
     float plane_x, plane_z;       // camera plane
     float pitch;                  // vertical angle
     float height;                 // height from floor
 } Camera;
 
 // map structure
 typedef struct {
     int width, height;
     char cells[MAP_SIZE][MAP_SIZE];
 } Map;
 
 // engine state structure
 typedef struct {
     int fullscreen;          
     float rotation_speed;    
     float move_speed;        
     int texture_quality;     
     int enable_filtering;    
     int enable_antialiasing;
     
     // fog settings
     int enable_fog;
     float fog_density;
     int fog_r, fog_g, fog_b;
     float fog_start_distance;
     float fog_end_distance;
     
     // lighting settings
     int enable_colored_lighting;
     int ambient_light_r, ambient_light_g, ambient_light_b;
     float default_light_intensity;
     
     // room settings
     int default_room_height;
     int ceiling_texture_id;
     int floor_texture_id;
     
     // advanced rendering
     float texture_scaling;
     int enable_shadows;
     float shadow_intensity;
     int render_distance;
 } EngineConfig;
 
 // global variables
 EngineConfig config;
 GLuint texture_ids[6]; 
 int running = 1;
 int keys[256] = {0}; 
 float camera_rotation = 0.0f;
 Camera camera;
 Map world_map;
 
 // forward declarations
 void init_opengl(void);
 void load_config(void);
 void save_config(void);
 void load_map(void);
 void generate_noise_textures(void);
 void apply_texture_filtering(void);
 void setup_lighting(void);
 void render_scene(void);
 void cleanup(void);
 void keyboard_func(unsigned char key, int x, int y);
 void keyboard_up_func(unsigned char key, int x, int y);
 void passive_motion_func(int x, int y);
 void init_camera(void);
 void update_camera_direction(void);
 int check_collision(float x, float z);
 void setup_fog(void);
 void init_perlin(void);
 
 // perlin noise generation prototypes
 float perlin_noise_2d(float x, float y);
 void generate_perlin_texture(unsigned char* buffer, int width, int height, int face);
 
 // input handling
 void handle_keyboard(void);
 void handle_mouse(void);
 
 /**
  * main entry point
  */
 int main(int argc, char** argv) {
     // seed random number generator
     srand((unsigned int)time(NULL));
     
     // init GLUT
     glutInit(&argc, argv);
     glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
     glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
     glutCreateWindow(WINDOW_TITLE);
     
     // load config
     load_config();
     
     // init OpenGL
     init_opengl();
     
     // init camera
     init_camera();
     
     // load map data
     load_map();
     
     // generate procedural textures
     generate_noise_textures();
     
     // set up callbacks
     glutDisplayFunc(render_scene);
     glutIdleFunc(render_scene);
     glutKeyboardFunc(keyboard_func);
     glutKeyboardUpFunc(keyboard_up_func);
     glutPassiveMotionFunc(passive_motion_func);
 
     // main loop
     glutMainLoop();
     
     // clean up
     cleanup();
     
     return 0;
 }
 
 /**
  * initialize OpenGL settings
  */
 void init_opengl(void) {
     // basic OpenGL setup
     glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
     glClearDepth(1.0f);
     glEnable(GL_DEPTH_TEST);
     glDepthFunc(GL_LEQUAL);
     
     // viewport setup
     glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
     glMatrixMode(GL_PROJECTION);
     glLoadIdentity();
     gluPerspective(45.0f, (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, (GLfloat)config.render_distance);
     glMatrixMode(GL_MODELVIEW);
     
     // enable textures
     glEnable(GL_TEXTURE_2D);
     
     // set up lighting
     setup_lighting();
     
     // set up fog if enabled
     if (config.enable_fog) {
         setup_fog();
     }
     
     // generate texture IDs
     glGenTextures(6, texture_ids);
 }
 
/**
 * save configuration to INI file
 */
void save_config(void) {
    FILE *file = fopen(CONFIG_FILE, "w");
    if (!file) {
        fprintf(stderr, "ERROR: Could not save configuration to '%s'\n", CONFIG_FILE);
        return;
    }
    
    // Write header comment
    fprintf(file, "# MOON Engine Configuration File\n");
    fprintf(file, "# Auto-generated on %s\n\n", __DATE__);
    
    // Basic settings
    fprintf(file, "fullscreen=%d\n", config.fullscreen);
    fprintf(file, "rotation_speed=%f\n", config.rotation_speed);
    fprintf(file, "move_speed=%f\n", config.move_speed);
    fprintf(file, "texture_quality=%d\n", config.texture_quality);
    fprintf(file, "enable_filtering=%d\n", config.enable_filtering);
    fprintf(file, "enable_antialiasing=%d\n", config.enable_antialiasing);
    
    // Fog settings
    fprintf(file, "\n# Fog settings\n");
    fprintf(file, "enable_fog=%d\n", config.enable_fog);
    fprintf(file, "fog_density=%f\n", config.fog_density);
    fprintf(file, "fog_r=%d\n", config.fog_r);
    fprintf(file, "fog_g=%d\n", config.fog_g);
    fprintf(file, "fog_b=%d\n", config.fog_b);
    fprintf(file, "fog_start_distance=%f\n", config.fog_start_distance);
    fprintf(file, "fog_end_distance=%f\n", config.fog_end_distance);
    
    // Lighting settings
    fprintf(file, "\n# Lighting settings\n");
    fprintf(file, "enable_colored_lighting=%d\n", config.enable_colored_lighting);
    fprintf(file, "ambient_light_r=%d\n", config.ambient_light_r);
    fprintf(file, "ambient_light_g=%d\n", config.ambient_light_g);
    fprintf(file, "ambient_light_b=%d\n", config.ambient_light_b);
    fprintf(file, "default_light_intensity=%f\n", config.default_light_intensity);
    
    // Room settings
    fprintf(file, "\n# Room settings\n");
    fprintf(file, "default_room_height=%d\n", config.default_room_height);
    fprintf(file, "ceiling_texture_id=%d\n", config.ceiling_texture_id);
    fprintf(file, "floor_texture_id=%d\n", config.floor_texture_id);
    
    // Advanced rendering
    fprintf(file, "\n# Advanced rendering\n");
    fprintf(file, "texture_scaling=%f\n", config.texture_scaling);
    fprintf(file, "enable_shadows=%d\n", config.enable_shadows);
    fprintf(file, "shadow_intensity=%f\n", config.shadow_intensity);
    fprintf(file, "render_distance=%d\n", config.render_distance);
    
    fclose(file);
    printf("Configuration saved to '%s'\n", CONFIG_FILE);
}

 /**
 * load configuration from INI file
 */
void load_config(void) {
    FILE *file = fopen(CONFIG_FILE, "r");
    
    // default configuration
    config.fullscreen = 0;
    config.rotation_speed = 1.0f;
    config.move_speed = 0.1f;
    config.texture_quality = 64;  // Size of noise textures
    config.enable_filtering = 1;
    config.enable_antialiasing = 1;
    
    // fog defaults
    config.enable_fog = 1;
    config.fog_density = 0.03f;
    config.fog_r = 128;
    config.fog_g = 128;
    config.fog_b = 200;
    config.fog_start_distance = 5.0f;
    config.fog_end_distance = 15.0f;
    
    // lighting defaults
    config.enable_colored_lighting = 1;
    config.ambient_light_r = 20;
    config.ambient_light_g = 20;
    config.ambient_light_b = 20;
    config.default_light_intensity = 0.8f;
    
    // room defaults
    config.default_room_height = 1;
    config.ceiling_texture_id = 1;
    config.floor_texture_id = 2;
    
    // advanced rendering defaults
    config.texture_scaling = 1.0f;
    config.enable_shadows = 0;
    config.shadow_intensity = 0.5f;
    config.render_distance = 20;
    
    if (file) {
        char line[128];
        char key[64];
        char value[64];
        
        while (fgets(line, sizeof(line), file)) {
            if (line[0] == ';' || line[0] == '#') // skip comments
                continue;
                
            if (sscanf(line, "%[^=]=%s", key, value) == 2) {
                // basic settings
                if (strcmp(key, "fullscreen") == 0) {
                    config.fullscreen = atoi(value);
                } else if (strcmp(key, "rotation_speed") == 0) {
                    config.rotation_speed = (float)atof(value);
                } else if (strcmp(key, "move_speed") == 0) {
                    config.move_speed = (float)atof(value);
                } else if (strcmp(key, "texture_quality") == 0) {
                    config.texture_quality = atoi(value);
                } else if (strcmp(key, "enable_filtering") == 0) {
                    config.enable_filtering = atoi(value);
                } else if (strcmp(key, "enable_antialiasing") == 0) {
                    config.enable_antialiasing = atoi(value);
                } 
                // fog settings
                else if (strcmp(key, "enable_fog") == 0) {
                    config.enable_fog = atoi(value);
                } else if (strcmp(key, "fog_density") == 0) {
                    config.fog_density = (float)atof(value);
                } else if (strcmp(key, "fog_r") == 0) {
                    config.fog_r = atoi(value);
                } else if (strcmp(key, "fog_g") == 0) {
                    config.fog_g = atoi(value);
                } else if (strcmp(key, "fog_b") == 0) {
                    config.fog_b = atoi(value);
                } else if (strcmp(key, "fog_start_distance") == 0) {
                    config.fog_start_distance = (float)atof(value);
                } else if (strcmp(key, "fog_end_distance") == 0) {
                    config.fog_end_distance = (float)atof(value);
                }
                // lighting settings
                else if (strcmp(key, "enable_colored_lighting") == 0) {
                    config.enable_colored_lighting = atoi(value);
                } else if (strcmp(key, "ambient_light_r") == 0) {
                    config.ambient_light_r = atoi(value);
                } else if (strcmp(key, "ambient_light_g") == 0) {
                    config.ambient_light_g = atoi(value);
                } else if (strcmp(key, "ambient_light_b") == 0) {
                    config.ambient_light_b = atoi(value);
                } else if (strcmp(key, "default_light_intensity") == 0) {
                    config.default_light_intensity = (float)atof(value);
                }
                // room settings
                else if (strcmp(key, "default_room_height") == 0) {
                    config.default_room_height = atoi(value);
                } else if (strcmp(key, "ceiling_texture_id") == 0) {
                    config.ceiling_texture_id = atoi(value);
                } else if (strcmp(key, "floor_texture_id") == 0) {
                    config.floor_texture_id = atoi(value);
                }
                // advanced rendering
                else if (strcmp(key, "texture_scaling") == 0) {
                    config.texture_scaling = (float)atof(value);
                } else if (strcmp(key, "enable_shadows") == 0) {
                    config.enable_shadows = atoi(value);
                } else if (strcmp(key, "shadow_intensity") == 0) {
                    config.shadow_intensity = (float)atof(value);
                } else if (strcmp(key, "render_distance") == 0) {
                    config.render_distance = atoi(value);
                }
            }
        }
        fclose(file);
        printf("Configuration loaded from '%s'\n", CONFIG_FILE);
    }
}
 
 /**
  * clean up resources
  */
 void cleanup(void) {
     // delete textures
     glDeleteTextures(6, texture_ids);
     
     // save configuration
     save_config();
 }
 
 /**
  * generate perlin noise texture
  */
 void generate_perlin_texture(unsigned char* buffer, int width, int height, int face) {
     int x, y;
     float scale = 0.1f * (face + 1); // Different scale per face
     
     for (y = 0; y < height; y++) {
         for (x = 0; x < width; x++) {
             // generate noise value
             float noise = perlin_noise_2d(x * scale, y * scale);
             
             // convert to grayscale color (temporary)
             unsigned char value = (unsigned char)(noise * 255);
             
             // RGB components
             buffer[(y * width + x) * 3 + 0] = value; // R
             buffer[(y * width + x) * 3 + 1] = value; // G
             buffer[(y * width + x) * 3 + 2] = value; // B
         }
     }
     
     // apply wu anti-aliasing if enabled
     if (config.enable_antialiasing) {
         // simple wu-inspired anti-aliasing for textures
         int x, y;
         unsigned char *temp = (unsigned char*)malloc(width * height * 3);
         if (temp) {
             // copy original buffer
             memcpy(temp, buffer, width * height * 3);
             
             // process interior pixels with a simple blur filter
             for (y = 1; y < height-1; y++) {
                 for (x = 1; x < width-1; x++) {
                     int idx = (y * width + x) * 3;
                     // check if this pixel is an edge
                     int c1 = temp[idx];
                     int c2 = temp[idx - 3];  // left
                     int c3 = temp[idx + 3];  // right
                     int c4 = temp[idx - width*3]; // up
                     int c5 = temp[idx + width*3]; // down
                     
                     // if we detect a significant change in intensity, apply smoothing
                     if (abs(c1-c2) > 32 || abs(c1-c3) > 32 || abs(c1-c4) > 32 || abs(c1-c5) > 32) {
                         // simple 3x3 weighted average for edge softening
                         buffer[idx] = buffer[idx+1] = buffer[idx+2] = 
                             (c1*4 + c2 + c3 + c4 + c5) / 8;
                     }
                 }
             }
             free(temp);
         }
     }
 }
 

/**
 * generate procedural noise textures
 */
void generate_noise_textures(void) {
    int i, width, height;
    unsigned char *texture_data;
    
    // initialize perlin noise
    init_perlin();
    
    width = height = config.texture_quality;
    texture_data = (unsigned char*)malloc(width * height * 3); // RGB data
    
    if (!texture_data) {
        fprintf(stderr, "ERROR: Could not allocate memory for textures!\n");
        return;
    }
    
    // generate different textures for each surface type
    for (i = 0; i < 6; i++) {
        // generate the texture data
        generate_perlin_texture(texture_data, width, height, i);
        
        // bind and configure texture
        glBindTexture(GL_TEXTURE_2D, texture_ids[i]);
        
        // set texture parameters
        if (config.enable_filtering) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        } else {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }
        
        // upload texture data
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, 
                     GL_UNSIGNED_BYTE, texture_data);
    }
    
    // free temporary memory
    free(texture_data);
    
    printf("Generated %d procedural textures at %dx%d resolution\n", 
           6, width, height);
}

/**
 * render the scene
 */
void render_scene(void) {
    // process input
    handle_keyboard();
    handle_mouse();
    
    // clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    
    // set camera pos and orientation
    float pitch_rad = camera.pitch * 0.0174532925f; // convert to radians
    glRotatef(camera.pitch, 1.0f, 0.0f, 0.0f);     // pitch (x-axis rotation)
    gluLookAt(
        camera.pos_x, camera.pos_y, camera.pos_z,                  // camera position
        camera.pos_x + camera.dir_x, camera.pos_y, camera.pos_z + camera.dir_z,  // look target
        0.0f, 1.0f, 0.0f                           // up vector
    );
    
    // draw floor and ceiling
    glEnable(GL_TEXTURE_2D);
    
    // draw walls
    int x, z;
    for (z = 0; z < world_map.height; z++) {
        for (x = 0; x < world_map.width; x++) {
            if (world_map.cells[z][x] != '0') {
                // select texture based on wall type
                int wall_type = (world_map.cells[z][x] - '0') % 6;
                if (wall_type < 1) wall_type = 1; // Avoid texture 0
                
                glBindTexture(GL_TEXTURE_2D, texture_ids[wall_type - 1]);
                
                // draw wall block
                glBegin(GL_QUADS);
                
                // set material properties
                if (config.enable_colored_lighting) {
                    // customize colors based on wall type
                    float r = 0.8f, g = 0.8f, b = 0.8f;
                    switch (wall_type) {
                        case 1: r = 1.0f; g = 0.8f; b = 0.8f; break; // Reddish
                        case 2: r = 0.8f; g = 1.0f; b = 0.8f; break; // Greenish
                        case 3: r = 0.8f; g = 0.8f; b = 1.0f; break; // Bluish
                        case 4: r = 1.0f; g = 1.0f; b = 0.8f; break; // Yellowish
                        case 5: r = 0.8f; g = 1.0f; b = 1.0f; break; // Cyanish
                        case 6: r = 1.0f; g = 0.8f; b = 1.0f; break; // Magentaish
                    }
                    GLfloat mat_diffuse[] = { r, g, b, 1.0f };
                    glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
                }
                
                // north wall face
                glTexCoord2f(0.0f, 0.0f); glVertex3f(x, 0.0f, z);
                glTexCoord2f(1.0f, 0.0f); glVertex3f(x + 1.0f, 0.0f, z);
                glTexCoord2f(1.0f, 1.0f); glVertex3f(x + 1.0f, 1.0f, z);
                glTexCoord2f(0.0f, 1.0f); glVertex3f(x, 1.0f, z);
                
                // south wall face
                glTexCoord2f(0.0f, 0.0f); glVertex3f(x, 0.0f, z + 1.0f);
                glTexCoord2f(1.0f, 0.0f); glVertex3f(x + 1.0f, 0.0f, z + 1.0f);
                glTexCoord2f(1.0f, 1.0f); glVertex3f(x + 1.0f, 1.0f, z + 1.0f);
                glTexCoord2f(0.0f, 1.0f); glVertex3f(x, 1.0f, z + 1.0f);
                
                // east wall face
                glTexCoord2f(0.0f, 0.0f); glVertex3f(x + 1.0f, 0.0f, z);
                glTexCoord2f(1.0f, 0.0f); glVertex3f(x + 1.0f, 0.0f, z + 1.0f);
                glTexCoord2f(1.0f, 1.0f); glVertex3f(x + 1.0f, 1.0f, z + 1.0f);
                glTexCoord2f(0.0f, 1.0f); glVertex3f(x + 1.0f, 1.0f, z);
                
                // west wall face
                glTexCoord2f(0.0f, 0.0f); glVertex3f(x, 0.0f, z);
                glTexCoord2f(1.0f, 0.0f); glVertex3f(x, 0.0f, z + 1.0f);
                glTexCoord2f(1.0f, 1.0f); glVertex3f(x, 1.0f, z + 1.0f);
                glTexCoord2f(0.0f, 1.0f); glVertex3f(x, 1.0f, z);
                
                glEnd();
            }
        }
    }
    
    // draw floor
    glBindTexture(GL_TEXTURE_2D, texture_ids[config.floor_texture_id - 1]);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(0.0f, 0.0f, 0.0f);
    glTexCoord2f(world_map.width, 0.0f); glVertex3f(world_map.width, 0.0f, 0.0f);
    glTexCoord2f(world_map.width, world_map.height); glVertex3f(world_map.width, 0.0f, world_map.height);
    glTexCoord2f(0.0f, world_map.height); glVertex3f(0.0f, 0.0f, world_map.height);
    glEnd();
    
    // draw ceiling
    glBindTexture(GL_TEXTURE_2D, texture_ids[config.ceiling_texture_id - 1]);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(0.0f, 1.0f, 0.0f);
    glTexCoord2f(world_map.width, 0.0f); glVertex3f(world_map.width, 1.0f, 0.0f);
    glTexCoord2f(world_map.width, world_map.height); glVertex3f(world_map.width, 1.0f, world_map.height);
    glTexCoord2f(0.0f, world_map.height); glVertex3f(0.0f, 1.0f, world_map.height);
    glEnd();
    
    // swap buffers to display the scene
    glutSwapBuffers();
}

 /**
  * set up lighting
  */
 void setup_lighting(void) {
     // simple lighting setup
     GLfloat light_ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
     GLfloat light_diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
     GLfloat light_position[] = { 0.0f, 0.0f, 2.0f, 1.0f };
     
     glEnable(GL_LIGHTING);
     glEnable(GL_LIGHT0);
     glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
     glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
     glLightfv(GL_LIGHT0, GL_POSITION, light_position);
 }
 
 /**
  * initialize camera
  */
 void init_camera(void) {
     camera.pos_x = 1.5f;
     camera.pos_z = 1.5f;
     camera.pos_y = 0.5f;  // eye level
     camera.height = 0.5f;
     camera.dir_x = -1.0f;
     camera.dir_z = 0.0f;
     camera.plane_x = 0.0f;
     camera.plane_z = 0.66f;
     camera.pitch = 0.0f;
     
     // initial direction update
     update_camera_direction();
 }
 
 /**
  * update camera direction based on rotation
  */
 void update_camera_direction(void) {
     float rad = camera_rotation * 0.0174532925f; // convert to radians
     
     camera.dir_x = cosf(rad);
     camera.dir_z = sinf(rad);
     
     // update camera plane perpendicular to direction
     camera.plane_x = -camera.dir_z * 0.66f;
     camera.plane_z = camera.dir_x * 0.66f;
 }
 
 /**
  * check if position collides with a wall
  * returns 1 if collision, 0 if no collision
  */
 int check_collision(float x, float z) {
     // convert to map coordinates
     int map_x = (int)x;
     int map_z = (int)z;
     
     // boundary check
     if (map_x < 0 || map_x >= world_map.width || 
         map_z < 0 || map_z >= world_map.height) {
         return 1;  // out of bounds is a collision
     }
     
     // collision with walls (any non-zero character)
     if (world_map.cells[map_z][map_x] != '0') {
         return 1;
     }
     
     return 0;
 }

 /**
 * set up fog effects
 */
void setup_fog(void) {
    GLfloat fog_color[4] = {
        config.fog_r / 255.0f,
        config.fog_g / 255.0f,
        config.fog_b / 255.0f,
        1.0f
    };
    
    // Enable fog
    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_LINEAR);  // Use linear fog mode
    glFogfv(GL_FOG_COLOR, fog_color);
    glFogf(GL_FOG_DENSITY, config.fog_density);
    glHint(GL_FOG_HINT, GL_NICEST);  // Use best fog quality
    
    // Set fog start and end distances
    glFogf(GL_FOG_START, config.fog_start_distance);
    glFogf(GL_FOG_END, config.fog_end_distance);
}
 
 /**
  * load map data
  */
 void load_map(void) {
     FILE *file;
     char line[256];
     int row = 0, col = 0;
     
     //initialize map with default values
     world_map.width = 10;
     world_map.height = 10;
     
     //fill with walls by default
     for (row = 0; row < MAP_SIZE; row++) {
         for (col = 0; col < MAP_SIZE; col++) {
             world_map.cells[row][col] = '1';
         }
     }
     
     printf("LOADING MAP FROM %s...\n", MAP_FILE);
     file = fopen(MAP_FILE, "r");
     if (!file) {
         fprintf(stderr, "ERROR: COULDN'T OPEN MAP FILE! '%s'\n", MAP_FILE);
         //create a simple default map
         printf("CREATING DEFAULT MAP...\n");
         
         //default map with a simple room
         world_map.width = 10;
         world_map.height = 10;
         
         //create walls around the edge, empty in the middle
         for (row = 0; row < world_map.height; row++) {
             for (col = 0; col < world_map.width; col++) {
                 if (row == 0 || col == 0 || row == world_map.height-1 || col == world_map.width-1) {
                     world_map.cells[row][col] = '1'; // Wall
                 } else {
                     world_map.cells[row][col] = '0'; // Empty space
                 }
             }
         }
         return;
     }
     
     // read map dimensions
     int width = 0, height = 0;
     if (fgets(line, sizeof(line), file) != NULL) {
         sscanf(line, "%d %d", &width, &height);
         printf("Map dimensions: %dx%d\n", width, height);
         
         if (width > 0 && width <= MAP_SIZE && height > 0 && height <= MAP_SIZE) {
             world_map.width = width;
             world_map.height = height;
         }
         
         // read map data
         row = 0;
         while (fgets(line, sizeof(line), file) && row < world_map.height) {
             col = 0;
             while (line[col] != '\0' && line[col] != '\n' && col < world_map.width) {
                 world_map.cells[row][col] = line[col];
                 col++;
             }
             row++;
         }
     }
     
     fclose(file);
     printf("MAP LOADED SUCCESSFULLY!\n");
 }
 
 //keyboard callback func
 void keyboard_func(unsigned char key, int x, int y) {
     //set the key state to 'PRESSED'
     keys[key] = 1;
 
     //handle ESC key first
     if (key == 27) {
         printf("EXITING PROGRAM...!\n");
     }
 }
 
 //keyboard up callback func
 void keyboard_up_func(unsigned char key, int x, int y) {
     //set the key state to 'RELEASED'
     keys[key] = 0;
 }
 
 //passive motion callback func
 void passive_motion_func(int x, int y) {
     static int last_x = WINDOW_WIDTH / 2;
     static int last_y = WINDOW_HEIGHT / 2;
     static int first_time = 1;
     
     if (first_time) {
         last_x = x;
         last_y = y; 
         first_time = 0;
         return;
     }
 
    //calculate delta from last pos
     int delta_x = x - last_x;
     int delta_y = y - last_y;
 
     // Horizontal rotation (yaw)
     if (delta_x != 0) {
         camera_rotation += delta_x * config.rotation_speed * 0.1f;
         update_camera_direction();
     }
     
     //vertical rotation (pitch) - limited to avoid flipping
     if (delta_y != 0) {
         camera.pitch += delta_y * config.rotation_speed * 0.1f;
         // Limit pitch to avoid gimbal lock
         if (camera.pitch > 89.0f) camera.pitch = 89.0f;
         if (camera.pitch < -89.0f) camera.pitch = -89.0f;
     }
 
     //store last position
     last_x = x;
     last_y = y;
     
     //if significant movement occurred, request redraw
     if (delta_x != 0 || delta_y != 0) {
         glutPostRedisplay();
     }
 
     if (config.fullscreen) {
         //center mouse in fullscreen mode 
         glutWarpPointer(WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);
     }
 }
 
 /**
  * handle keyboard input
  */
 void handle_keyboard(void) {
     //process keyboard state
     int modifiers = glutGetModifiers();
     static int last_time = 0;
     int current_time = glutGet(GLUT_ELAPSED_TIME);
     
     //limit input checking to avoid excessive CPU usage
     if (current_time - last_time < 16) {
         return;
     }
     last_time = current_time;
 
     if (modifiers & GLUT_ACTIVE_ALT) {
         //alt key combos
         if (keys['f'] || keys['F']) {
             //fullscreen
             config.fullscreen = !config.fullscreen;
             keys['f'] = keys['F'] = 0; // Clear key state
 
             if (config.fullscreen) {
                 glutFullScreen();
             } else {
                 glutReshapeWindow(WINDOW_WIDTH, WINDOW_HEIGHT);
                 glutPositionWindow(50, 50);
             }
         }
 
         if (keys['s'] || keys['S']) {
             //save config
             save_config();
             keys['s'] = keys['S'] = 0; // Clear key state
             printf("CONFIG SAVED...\n");
         }
     } else {
         //normal move keys
         float move_speed = config.move_speed;
         float new_x, new_z;
         
         //forward/back
         if (keys['w'] || keys['W']) {
             new_x = camera.pos_x + camera.dir_x * move_speed;
             new_z = camera.pos_z + camera.dir_z * move_speed;
             if (!check_collision(new_x, new_z)) {
                 camera.pos_x = new_x;
                 camera.pos_z = new_z;
             }
         }
         if (keys['s'] || keys['S']) {
             new_x = camera.pos_x - camera.dir_x * move_speed;
             new_z = camera.pos_z - camera.dir_z * move_speed;
             if (!check_collision(new_x, new_z)) {
                 camera.pos_x = new_x;
                 camera.pos_z = new_z;
             }
         }
 
         //strafe left and right
         if (keys['a'] || keys['A']) {
             //move left
             new_x = camera.pos_x - camera.plane_x * move_speed;
             new_z = camera.pos_z - camera.plane_z * move_speed;
             if (!check_collision(new_x, new_z)) {
                 camera.pos_x = new_x;
                 camera.pos_z = new_z;
             }
         }
         if (keys['d'] || keys['D']) {
             //move right
             new_x = camera.pos_x + camera.plane_x * move_speed;
             new_z = camera.pos_z + camera.plane_z * move_speed;
             if (!check_collision(new_x, new_z)) {
                 camera.pos_x = new_x;
                 camera.pos_z = new_z;
             }
         }
         
         //rotation
         if (keys['q'] || keys['Q']) {
             //rotate left
             camera_rotation += config.rotation_speed * 2.0f;
             update_camera_direction();
         }
         if (keys['e'] || keys['E']) {
             //rotate right
             camera_rotation -= config.rotation_speed * 2.0f;
             update_camera_direction();
         }
     }
     
     //only request redisplay if any keys were pressed
     if (keys['w'] || keys['a'] || keys['s'] || keys['d'] || 
         keys['q'] || keys['e'] || keys['W'] || keys['A'] || 
         keys['S'] || keys['D'] || keys['Q'] || keys['E']) {
         glutPostRedisplay();
     }
 }
 
 /**
  * handle mouse input
  */
 void handle_mouse(void) {
     //get mouse position
     int x, y;
     static int last_x = WINDOW_WIDTH / 2;
     static int last_y = WINDOW_HEIGHT / 2;
     
     x = glutGet(GLUT_WINDOW_WIDTH) / 2;
     y = glutGet(GLUT_WINDOW_HEIGHT) / 2;
     
     if (config.fullscreen) {
         glutWarpPointer(x, y);
     }
 }
 
 //perlin noise implementation
 float fade(float t) {
     return t * t * t * (t * (t * 6 - 15) + 10);
 }
 
 float lerp(float a, float b, float t) {
     return a + t * (b - a);
 }
 
 float grad(int hash, float x, float y, float z) {
     int h = hash & 15;
     float u = h < 8 ? x : y;
     float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
     return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
 }
 
 //permutation table
 static int p[512];
 void init_perlin() {
     int i;
     //initialize table
     for (i = 0; i < 256; i++) {
         p[i] = i;
     }
     
     //shuffle array
     for (i = 255; i > 0; i--) {
         int j = rand() % (i + 1);
         int temp = p[i];
         p[i] = p[j];
         p[j] = temp;
     }
     
     //duplicate
     for (i = 0; i < 256; i++) {
         p[i + 256] = p[i];
     }
 }
 
 float perlin_noise_2d(float x, float y) {
     //add a small value to ensure non-zero input
     y += 0.01f;
     
     //convert to 3D by using y as both y and z
     int X = (int)floor(x) & 255;
     int Y = (int)floor(y) & 255;
     int Z = (int)floor(y) & 255;
     
     x -= floor(x);
     y -= floor(y);
     float z = y;
     
     float u = fade(x);
     float v = fade(y);
     float w = fade(z);
     
     int A = p[X] + Y;
     int AA = p[A] + Z;
     int AB = p[A + 1] + Z;
     int B = p[X + 1] + Y;
     int BA = p[B] + Z;
     int BB = p[B + 1] + Z;
     
     //blending
     return lerp(
         lerp(
             lerp(grad(p[AA], x, y, z), grad(p[BA], x-1, y, z), u),
             lerp(grad(p[AB], x, y-1, z), grad(p[BB], x-1, y-1, z), u),
             v),
         lerp(
             lerp(grad(p[AA+1], x, y, z-1), grad(p[BA+1], x-1, y, z-1), u),
             lerp(grad(p[AB+1], x, y-1, z-1), grad(p[BB+1], x-1, y-1, z-1), u),
             v),
         w);
 }