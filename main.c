/**
 * MOON Engine - minimalist OpenGL open-source engine
 * lightweight and customizable game engine targeting older systems
 * designed for Windows 98/XP, Mac, and Linux compatibility
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* use older OpenGL for maximum compatibility */
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>

/* platform-specific includes */
#ifdef _WIN32
#include <GL/glaux.h>
#elif defined(__APPLE__)
#include <OpenGL/gl.h>
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

/* configuration constants */
#define WINDOW_WIDTH 320
#define WINDOW_HEIGHT 240
#define WINDOW_TITLE "MOON Engine v0.1"
#define CONFIG_FILE "config.ini"
#define MAP_FILE "map.txt"

/* engine state structure */
typedef struct {
    int fullscreen;          
    float rotation_speed;    
    float move_speed;        
    int texture_quality;     
    int enable_filtering;    
    int enable_antialiasing; 
} EngineConfig;

/* global variables */
EngineConfig config;
GLuint texture_ids[6]; 
int running = 1;
int keys[256] = {0}; 
float camera_rotation = 0.0f;

/* forward declarations */
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


/* perlin noise generation prototypes */
float perlin_noise_2d(float x, float y);
void generate_perlin_texture(unsigned char* buffer, int width, int height, int face);

/* input handling */
void handle_keyboard(void);
void handle_mouse(void);

/**
 * main entry point
 */
int main(int argc, char** argv) {
    /* seed random number generator */
    srand((unsigned int)time(NULL));
    
    /* init GLUT */
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow(WINDOW_TITLE);
    
    /* load config */
    load_config();
    
    /* init OpenGL */
    init_opengl();
    
    /* load map data */
    load_map();
    
    /* generate procedural textures */
    generate_noise_textures();
    
    /* set up callbacks */
    glutDisplayFunc(render_scene);
    glutIdleFunc(render_scene);
    glutKeyboardFunc(keyboard_func);
    glutKeyboardUpFunc(keyboard_up_func);
    glutPassiveMotionFunc(passive_motion_func);

    /* main loop */
    glutMainLoop();
    
    /* clean up */
    cleanup();
    
    return 0;
}

/**
 * initialize OpenGL settings
 */
void init_opengl(void) {
    /* basic OpenGL setup */
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    
    /* viewport setup */
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
    
    /* enable textures */
    glEnable(GL_TEXTURE_2D);
    
    /* set up lighting */
    setup_lighting();
    
    /* generate texture IDs */
    glGenTextures(6, texture_ids);
}

/**
 * load configuration from INI file
 */
void load_config(void) {
    FILE *file = fopen(CONFIG_FILE, "r");
    
    /* default configuration */
    config.fullscreen = 0;
    config.rotation_speed = 1.0f;
    config.move_speed = 0.1f;
    config.texture_quality = 64;  /* Size of noise textures */
    config.enable_filtering = 1;
    config.enable_antialiasing = 1;
    
    if (file) {
        char line[128];
        char key[64];
        char value[64];
        
        while (fgets(line, sizeof(line), file)) {
            if (sscanf(line, "%[^=]=%s", key, value) == 2) {
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
            }
        }
        
        fclose(file);
    } else {
        /* configuration file not found, create it */
        save_config();
    }
    
    /* apply fullscreen if configured */
    if (config.fullscreen) {
        glutFullScreen();
    }
}

/**
 * save configuration to INI file
 */
void save_config(void) {
    FILE *file = fopen(CONFIG_FILE, "w");
    
    if (file) {
        fprintf(file, "fullscreen=%d\n", config.fullscreen);
        fprintf(file, "rotation_speed=%f\n", config.rotation_speed);
        fprintf(file, "move_speed=%f\n", config.move_speed);
        fprintf(file, "texture_quality=%d\n", config.texture_quality);
        fprintf(file, "enable_filtering=%d\n", config.enable_filtering);
        fprintf(file, "enable_antialiasing=%d\n", config.enable_antialiasing);
        fclose(file);
    }
}

/**
 * generate procedural noise textures
 */
void generate_noise_textures(void) {
    int i;
    int size = config.texture_quality;
    unsigned char* texture_data = (unsigned char*)malloc(size * size * 3);
    
    if (!texture_data) {
        fprintf(stderr, "Failed to allocate texture memory\n");
        return;
    }
    
    for (i = 0; i < 6; i++) {
        /* generate unique noise for each cube face */
        generate_perlin_texture(texture_data, size, size, i);
        
        /* bind and upload texture */
        glBindTexture(GL_TEXTURE_2D, texture_ids[i]);
        
        /* apply texture filtering based on config */
        if (config.enable_filtering) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        } else {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }
        
        /* upload the texture */
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size, size, 0, 
                    GL_RGB, GL_UNSIGNED_BYTE, texture_data);
    }
    
    free(texture_data);
}

/**
 * render a frame
 */
void render_scene(void) {
    /* Clear the screen and depth buffer */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
     
    /* position camera */
    gluLookAt(
        0.0f, 0.0f, 5.0f,  /* Eye position */
        0.0f, 0.0f, 0.0f,  /* Look at position */
        0.0f, 1.0f, 0.0f   /* Up vector */
    );
    
    //enable texturing
    glEnable(GL_TEXTURE_2D);

    //front face
    glBindTexture(GL_TEXTURE_2D, texture_ids[0]);
    glBegin(GL_QUADS);
    	glNormal3f(0.0f, 0.0f, 1.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(1.0f -1.0f, 1.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f, 1.0f, 1.0f);
    glEnd();

    //back face 
    glBindTexture(GL_TEXTURE_2D, texture_ids[1]);
    glBegin(GL_QUADS);
    	glNormal3f(0.0f, 0.0f, -1.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f 1.0f, 1.0f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(1.0f, -1.0f, -1.0f);
    glEnd();

    //top face 
    glBindTexture(GL_TEXTURE_2D, texture_ids[2]);
    glBegin(GL_QUADS);
    	glNormal3f(0.0f, 1.0f, 0.0f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f  1.0f,  1.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, 1.0f,  1.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f, 1.0f, -1.0f);
    glEnd();


    //bottom face
    glBindTexture(GL_TEXTURE_2D, texture_ids[3]);
    glBegin(GL_QUADS);
    	glNormal3f(0.0f, -1.0f, 0.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f, -1.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);i
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);
    glEnd();

    //right face
    glBindTexture(GL_TEXTURE_2D, texture_ids[4]);
    glBegin(GL_QUADS);
    	glNormal3f(1.0f, 0.0f, 0.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f, -1.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f, 1.0f, 1.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, 1.0f, 1.0f);
    glEnd();

    //left face
    glBindTexture(GL_TEXTURE_2D, texture_ids[5]);
    glBegin(GL_QUADS);
    	glNormal3f(0.0f, 0.0f, -1.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f, 1.0f, 1.0f);
    glEnd();
    /* Swap buffers */
    glutSwapBuffers();
    
    /* Handle input */
    handle_keyboard();
    handle_mouse();
}

/**
 * clean up resources
 */
void cleanup(void) {
    /* delete textures */
    glDeleteTextures(6, texture_ids);
    
    /* save configuration */
    save_config();
}

/* placeholder for Perlin noise implementation */
float perlin_noise_2d(float x, float y) {
    /* TODO: implement Perlin noise function */
    return (float)rand() / RAND_MAX;
}

/**
 * generate perlin noise texture
 */
void generate_perlin_texture(unsigned char* buffer, int width, int height, int face) {
    int x, y;
    float scale = 0.1f * (face + 1); /* Different scale per face */
    
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            /* generate noise value */
            float noise = perlin_noise_2d(x * scale, y * scale);
            
            /* convert to grayscale color (temporary) */
            unsigned char value = (unsigned char)(noise * 255);
            
            /* RGB components */
            buffer[(y * width + x) * 3 + 0] = value; /* R */
            buffer[(y * width + x) * 3 + 1] = value; /* G */
            buffer[(y * width + x) * 3 + 2] = value; /* B */
        }
    }
    
    /* apply wu anti-aliasing if enabled */
    if (config.enable_antialiasing) {
        /* simple wu-inspired anti-aliasing for textures */
        int x, y;
        unsigned char *temp = (unsigned char*)malloc(width * height * 3);
        if (temp) {
            /* copy original buffer */
            memcpy(temp, buffer, width * height * 3);
            
            /* process interior pixels with a simple blur filter */
            for (y = 1; y < height-1; y++) {
                for (x = 1; x < width-1; x++) {
                    int idx = (y * width + x) * 3;
                    /* check if this pixel is an edge */
                    int c1 = temp[idx];
                    int c2 = temp[idx - 3];  /* left */
                    int c3 = temp[idx + 3];  /* right */
                    int c4 = temp[idx - width*3]; /* up */
                    int c5 = temp[idx + width*3]; /* down */
                    
                    /* if we detect a significant change in intensity, apply smoothing */
                    if (abs(c1-c2) > 32 || abs(c1-c3) > 32 || abs(c1-c4) > 32 || abs(c1-c5) > 32) {
                        /* simple 3x3 weighted average for edge softening */
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
 * set up lighting
 */
void setup_lighting(void) {
    /* simple lighting setup */
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
 * load map data
 */
void load_map(void) {
    FILE *file;
    char line[256];
    int row = 0, col = 0;
    
    /* map format:
     * first line: width height (dimensions of the map)
     * subsequent lines: grid of cells where:
     *   0 = empty space
     *   1-9 = wall with different texture variants
     *   A-F = special objects/entities
     * ex:
     * 10 10
     * 1111111111
     * 1000000001
     * 1020000301
     * 1000000001
     * 1000A00001
     * 1000000001
     * 1040000501
     * 1000000001
     * 1000000001
     * 1111111111
     */
    
    printf("LOADING MAP FROM %s...!\n", MAP_FILE);
    file = fopen(MAP_FILE, "r");
    if (!file) {
        fprintf(stderr, "ERROR: COULDN'T OPEN MAP FILE! '%s'\n", MAP_FILE);
        /* Create a simple default map */
        printf("CREATING DEFAULT MAP...!\n");
        return;
    }
    
    /* read map dimensions */
    int width = 0, height = 0;
    if (fgets(line, sizeof(line), file) != NULL) {
        sscanf(line, "%d %d", &width, &height);
        printf("map dimensions: %dx%d\n", width, height);
        
        /* TODO: allocate memory for map data based on width and height
         * this will be used later when implementing the actual map renderer
         */
         
        /* read map data */
        while (fgets(line, sizeof(line), file) && row < height) {
            col = 0;
            while (line[col] != '\0' && line[col] != '\n' && col < width) {
                /* Process each character in the map
                 * For now, we just read the data and will use it later
                 * when implementing the actual renderer
                 */
                char cell = line[col];
                
                /* TODO: store map data in the allocated map structure */
                
                col++;
            }
            row++;
        }
    }
    
    fclose(file);
    printf("MAP LOADED SUCCESSFULLY!\n");
}

/**
 * handle keyboard input
 */
void handle_keyboard(void) {
    /* process keyboard state */
    unsigned char keys[256];
    int modifiers = glutGetModifiers();
    static int last_time = 0;
    int current_time = glutGet(GLUT_ELAPSED_TIME);
    
    /* limit input checking to avoid excessive CPU usage */
    if (current_time - last_time < 16) { /* ~60 fps rate limiting */
        return;
    }
    last_time = current_time;
    
    /* esc key exits program */
    if (glutKeyboardFunc(NULL) == 27) {
        printf("Exiting program\n");
        cleanup();
        exit(0);
    }

    /* handle WASD movement */
    if (glutGetModifiers() & GLUT_ACTIVE_ALT) {
        /* alt key combinations for engine controls */
        if (glutKeyboardFunc(NULL) == 'f' || glutKeyboardFunc(NULL) == 'F') {
            /* toggle fullscreen */
            config.fullscreen = !config.fullscreen;
            if (config.fullscreen) {
                glutFullScreen();
            } else {
                glutReshapeWindow(WINDOW_WIDTH, WINDOW_HEIGHT);
                glutPositionWindow(50, 50);
            }
        }
        
        if (glutKeyboardFunc(NULL) == 's' || glutKeyboardFunc(NULL) == 'S') {
            /* save configuration */
            save_config();
            printf("Configuration saved\n");
        }
    } else {
        /* normal movement keys */
        float move_amount = config.move_speed;
        
        /* forward/backward */
        if (glutKeyboardFunc(NULL) == 'w' || glutKeyboardFunc(NULL) == 'W') {
            /* move forward */
            /* TODO: update camera position */
        }
        if (glutKeyboardFunc(NULL) == 's' || glutKeyboardFunc(NULL) == 'S') {
            /* move backward */
            /* TODO: update camera position */
        }
        
        /* strafe left/right */
        if (glutKeyboardFunc(NULL) == 'a' || glutKeyboardFunc(NULL) == 'A') {
            /* strafe left */
            /* TODO: update camera position */
        }
        if (glutKeyboardFunc(NULL) == 'd' || glutKeyboardFunc(NULL) == 'D') {
            /* strafe right */
            /* TODO: update camera position */
        }
        
        /* rotation */
        if (glutKeyboardFunc(NULL) == 'q' || glutKeyboardFunc(NULL) == 'Q') {
            /* rotate left */
            /* TODO: Update camera rotation */
        }
        if (glutKeyboardFunc(NULL) == 'e' || glutKeyboardFunc(NULL) == 'E') {
            /* rotate right */
            /* TODO: update camera rotation */
        }
    }
    
    /* request redisplay */
    glutPostRedisplay();
}

/**
 * handle mouse input
 */
void handle_mouse(void) {
    /* get mouse position */
    int x, y;
    static int last_x = WINDOW_WIDTH / 2;
    static int last_y = WINDOW_HEIGHT / 2;
    
    x = glutGet(GLUT_WINDOW_X);
    y = glutGet(GLUT_WINDOW_Y);
    
    /* handle mouse movements for camera rotation */
    if (x != last_x || y != last_y) {
        /* Calculate delta */
        int delta_x = x - last_x;
        int delta_y = y - last_y;
        
        /* use delta to rotate camera (sensitivity controlled by rotation_speed) */
        if (delta_x != 0) {
            /* rotate camera horizontally */
            /* TODO: update camera yaw angle */
        }
        
        if (delta_y != 0) {
            /* rotate camera vertically */
            /* TODO: update camera pitch angle, with limits to prevent flipping */
        }
        
        /* reset mouse position to center in fullscreen mode to allow continuous rotation */
        if (config.fullscreen) {
            glutWarpPointer(WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);
            last_x = WINDOW_WIDTH / 2;
            last_y = WINDOW_HEIGHT / 2;
        } else {
            last_x = x;
            last_y = y;
        }
        
        /* request redisplay */
        glutPostRedisplay();
    }
}
