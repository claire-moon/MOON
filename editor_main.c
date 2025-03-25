// map editor

#include <stdio.h>
// claire notes!!!:
// put an index of color values up here sometime but not now lol whenever
// u want to ok no rush!! love ya!!
// --
//

#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define GRID_SIZE 64
#define CELL_SIZE 24
#define UI_PANEL_WIDTH 300

#define TOOL_OPEN 0
#define TOOL_WALL 1
#define TOOL_PLAYER 2
#define TOOL_LIGHT 3
#define TOOL_COLOR_PICKER 4 
#define TAB_PAINTER 0
#define TAB_RENDERING 1
#define TAB_TEXTURE 2
#define TAB_COMPILE 3

#define MAX_MAP_SIZE_OPTIONS 4
#define DIALOG_WIDTH 300
#define DIALOG_HEIGHT 150

#define MAX_TEXT_INPUT_LENGTH 2
#define MAX_INPUT_FIELDS 2

#define MAP_FILE_PATH "/home/instar/programming/MOON/map.txt"
#define MAX_COMPILE_BUTTONS 3

#define CONSOLE_HEIGHT 120
#define MAX_CONSOLE_LINES 100
#define MAX_CONSOLE_LINE_LENGTH 128

// color picker definitions
#define COLOR_PICKER_WIDTH 350
#define COLOR_PICKER_HEIGHT 250

typedef struct {
    SDL_Rect rect;
    char label[32];
    int active;
    SDL_Color color;
} UIButton;

typedef struct {
	SDL_Rect rect;
	char label[32];
} UITab;

typedef struct {
    char label[16];   
    int width;        
    int height;       
} MapSizeOption;

typedef struct {
    SDL_Rect rect;
    char text[MAX_TEXT_INPUT_LENGTH + 2]; // +2 because I'm Afraid So Very Very Afraid (Scared)
    int active;
    int cursorPosition;
    int maxLength;
    int selectionStart; // add selection support
    int selectionEnd;   // add selection support
} UITextInput;

typedef struct {
    int active;          
    char message[128];   
    SDL_Rect rect;       
    SDL_Rect okButton;   
    SDL_Rect cancelButton;
    void (*onConfirm)(void*);
    void* userData;     
} UIDialog;

// color picker structure
typedef struct {
    int active;
    SDL_Rect rect;
    SDL_Rect previewRect;
    SDL_Rect sliders[3];      
    UITextInput inputs[3];    
    SDL_Rect applyButton;
    SDL_Rect cancelButton;
    SDL_Color currentColor;   
    SDL_Color originalColor;  
} ColorPicker;

typedef struct {
    SDL_Rect rect;
    char label[32];
    SDL_Color color;
} CompileButton;

typedef struct {
    char lines[MAX_CONSOLE_LINES][MAX_CONSOLE_LINE_LENGTH];
    int line_count;
    int scroll_offset;
} Console;

typedef struct {
    int map_w, map_h;
    char cells[GRID_SIZE][GRID_SIZE];
    int selected_tool;
    int selected_block;
    int mouse_down;
    int panning;
    SDL_Point pan_offset;
    SDL_Point last_mouse;
    int current_tab;
    UITextInput sizeInputs[MAX_INPUT_FIELDS]; // width and height inputs
    UIButton applyButton;                     // apply button for size changes
    UIDialog confirmDialog;
    SDL_Point player_pos;                    // track player position
    int player_exists;                       // flag for player existence
    int light_placed;                        // track if light was placed in current click
    float zoom_factor;                       // zoom factor for the grid
    Console console;                         // debug console
    ColorPicker colorPicker;                 // color picker for lights
    SDL_Color lightColor;                    // current light color
} EditorState; 

SDL_Window* window;
SDL_Renderer* renderer;
TTF_Font* font;
EditorState editor;
UIButton tools[5];
UITab tabs[4];
CompileButton compileButtons[MAX_COMPILE_BUTTONS];

void init_editor();
void draw_grid();
void draw_blocks();
void draw_ui();
void handle_mouse(SDL_Event* e);
void center_grid();
void change_map_size(void* sizeIndex);
void draw_dialog(UIDialog* dialog);
void handle_dialog(UIDialog* dialog, int mx, int my);
void draw_text_input(UITextInput* input);
void handle_text_input(UITextInput* input, SDL_Event* e);
void apply_size_change();
int validate_digit_input(const char* text, char c);
int get_cursor_pos_from_mouse(UITextInput* input, int mouseX);
void update_text_selection(UITextInput* input, int mouseX, int isMouseDown);
void handle_mouse_wheel(SDL_Event* e);
void init_compile_buttons();
void draw_compile_buttons();
int check_file_exists(const char *filename);
void save_map_to_file();
void load_map_from_file();
void open_map_in_editor();
void show_map_not_found_dialog();
void load_default_map();
void draw_editor_title();
void draw_panel_logo();
void draw_console();
void append_to_console(const char* message);
void clear_console();
void init_color_picker();
void draw_color_picker(ColorPicker* picker);
void handle_color_picker(ColorPicker* picker, SDL_Event* e);
int is_light_color(SDL_Color color);

// initialize editor
void init_editor() {
    editor.map_w = 20;
    editor.map_h = 15;
    editor.selected_tool = 1;
    editor.player_exists = 0;
    editor.player_pos.x = -1;
    editor.player_pos.y = -1;
    editor.light_placed = 0;
    editor.zoom_factor = 1.0f; 
    
    // initialize default light color
    editor.lightColor = (SDL_Color){255, 255, 255, 255};
    
    // initialize color picker
    init_color_picker();
    
    // initialize text input fields for width and height
    int inputWidth = 45;
    int inputHeight = 30;
    int inputPadding = 10;
    int inputStartY = 60;
    
    editor.sizeInputs[0].rect = (SDL_Rect){
        WINDOW_WIDTH - UI_PANEL_WIDTH + 35,
        inputStartY,
        inputWidth,
        inputHeight
    };
    sprintf(editor.sizeInputs[0].text, "%d", editor.map_w);
    editor.sizeInputs[0].active = 0;
    editor.sizeInputs[0].cursorPosition = strlen(editor.sizeInputs[0].text);
    editor.sizeInputs[0].maxLength = MAX_TEXT_INPUT_LENGTH;
    editor.sizeInputs[0].selectionStart = -1;
    editor.sizeInputs[0].selectionEnd = -1;
    
    // height input field (right)
    editor.sizeInputs[1].rect = (SDL_Rect){
        WINDOW_WIDTH - UI_PANEL_WIDTH + UI_PANEL_WIDTH/2 + 5,
        inputStartY,
        inputWidth,
        inputHeight
    };
    sprintf(editor.sizeInputs[1].text, "%d", editor.map_h);
    editor.sizeInputs[1].active = 0;
    editor.sizeInputs[1].cursorPosition = strlen(editor.sizeInputs[1].text);
    editor.sizeInputs[1].maxLength = MAX_TEXT_INPUT_LENGTH;
    editor.sizeInputs[1].selectionStart = -1;
    editor.sizeInputs[1].selectionEnd = -1;

    // position apply beneath inputs
    editor.applyButton.rect = (SDL_Rect){
	WINDOW_WIDTH - UI_PANEL_WIDTH + inputPadding,
	inputStartY + inputHeight + 10,
	UI_PANEL_WIDTH - inputPadding * 2,
	30
    };
    strcpy(editor.applyButton.label, "APPLY");
    editor.applyButton.color = (SDL_Color){80, 80, 120, 255};

    // define button dimensions
    int button_width = UI_PANEL_WIDTH - 20;
    int button_height = 40;
    int button_padding = 10;
    int button_y_spacing = 10;

    int button_start_y = editor.applyButton.rect.y + editor.applyButton.rect.h + 30;

    // initialize confirmation dialog
    editor.confirmDialog.active = 0;
    strcpy(editor.confirmDialog.message, "");
    editor.confirmDialog.rect = (SDL_Rect){
        (WINDOW_WIDTH - DIALOG_WIDTH) / 2,
        (WINDOW_HEIGHT - DIALOG_HEIGHT) / 2,
        DIALOG_WIDTH,
        DIALOG_HEIGHT
    };
    editor.confirmDialog.okButton = (SDL_Rect){
        (WINDOW_WIDTH - DIALOG_WIDTH) / 2 + 50,
        (WINDOW_HEIGHT - DIALOG_HEIGHT) / 2 + 100,
        80,
        30
    };
    editor.confirmDialog.cancelButton = (SDL_Rect){
        (WINDOW_WIDTH - DIALOG_WIDTH) / 2 + DIALOG_WIDTH - 130,
        (WINDOW_HEIGHT - DIALOG_HEIGHT) / 2 + 100,
        80,
        30
    };
    
    center_grid();

    // init tools panel
    tools[TOOL_OPEN].rect = (SDL_Rect){
        WINDOW_WIDTH-UI_PANEL_WIDTH+button_padding, 
        button_start_y + (button_height + button_y_spacing) * 1,
        button_width, 
        button_height
    };
    strcpy(tools[TOOL_OPEN].label, "OPEN");
    tools[TOOL_OPEN].color = (SDL_Color){50, 50, 50, 255};

    tools[TOOL_WALL].rect = (SDL_Rect){
        WINDOW_WIDTH-UI_PANEL_WIDTH+button_padding, 
        button_start_y + (button_height + button_y_spacing) * 2,
        button_width, 
        button_height
    };
    strcpy(tools[TOOL_WALL].label, "WALL");
    tools[TOOL_WALL].color = (SDL_Color){80, 80, 80, 255};

    tools[TOOL_PLAYER].rect = (SDL_Rect){
        WINDOW_WIDTH-UI_PANEL_WIDTH+button_padding, 
        button_start_y + (button_height + button_y_spacing) * 3, 
        button_width, 
        button_height
    };
    strcpy(tools[TOOL_PLAYER].label, "PLAYER");
    tools[TOOL_PLAYER].color = (SDL_Color){255, 0, 0, 255};

    tools[TOOL_LIGHT].rect = (SDL_Rect){
        WINDOW_WIDTH-UI_PANEL_WIDTH+button_padding, 
        button_start_y + (button_height + button_y_spacing) * 4, 
        button_width, 
        button_height
    };
    strcpy(tools[TOOL_LIGHT].label, "LIGHT");
    tools[TOOL_LIGHT].color = editor.lightColor;

    tools[TOOL_COLOR_PICKER].rect = (SDL_Rect){
        WINDOW_WIDTH - UI_PANEL_WIDTH + button_padding,
        button_start_y + (button_height + button_y_spacing) * 5, 
        button_width,
        button_height
    };
    strcpy(tools[TOOL_COLOR_PICKER].label, "COLOR");
    tools[TOOL_COLOR_PICKER].color = (SDL_Color){100, 100, 100, 255};

    // init tabs
    int tab_width = 120;
    int tab_height = 30;
    int tab_padding = 5;
    int tab_start_x = WINDOW_WIDTH - UI_PANEL_WIDTH - tab_width;
    int tab_start_y = 50;

    tabs[TAB_PAINTER].rect = (SDL_Rect){tab_start_x, tab_start_y, tab_width, tab_height};
    strcpy(tabs[TAB_PAINTER].label, "PAINTER");

    tabs[TAB_RENDERING].rect = (SDL_Rect){tab_start_x, tab_start_y + tab_height + tab_padding, tab_width, tab_height};
    strcpy(tabs[TAB_RENDERING].label, "RENDERING");

    tabs[TAB_TEXTURE].rect = (SDL_Rect){tab_start_x, tab_start_y + (tab_height + tab_padding) * 2, tab_width, tab_height};
    strcpy(tabs[TAB_TEXTURE].label, "TEXTURE");

    tabs[TAB_COMPILE].rect = (SDL_Rect){tab_start_x, tab_start_y + (tab_height + tab_padding) * 3, tab_width, tab_height};
    strcpy(tabs[TAB_COMPILE].label, "COMPILE");

    // init map
    for(int y=0; y<GRID_SIZE; y++) {
        for(int x=0; x<GRID_SIZE; x++) {
            editor.cells[y][x] = (x == 0 || y == 0 || x == editor.map_w-1 || y == editor.map_h-1) ? '1' : '0';
        }
    }

    init_compile_buttons();

    // initialize console
    editor.console.line_count = 0;
    editor.console.scroll_offset = 0;
    append_to_console("MOON Editor v0.3 initialized");
    append_to_console("Console ready");
}

void center_grid() {
    float zoomed_cell_size = CELL_SIZE * editor.zoom_factor;
    
    // calculate the available height
    int available_height = WINDOW_HEIGHT - CONSOLE_HEIGHT;
    
    editor.pan_offset.x = (WINDOW_WIDTH - UI_PANEL_WIDTH - (editor.map_w * zoomed_cell_size)) / 2;
    editor.pan_offset.y = (available_height - (editor.map_h * zoomed_cell_size)) / 2;
}

// change map size
void change_map_size(void* userData) {
    int newWidth = atoi(editor.sizeInputs[0].text);
    int newHeight = atoi(editor.sizeInputs[1].text);
    
    // validate the input values
    if (newWidth < 5) newWidth = 5;
    if (newWidth > 99) newWidth = 99;
    if (newHeight < 5) newHeight = 5;
    if (newHeight > 99) newHeight = 99;
    
    // update the input field text to reflect validated values
    sprintf(editor.sizeInputs[0].text, "%d", newWidth);
    sprintf(editor.sizeInputs[1].text, "%d", newHeight);
    
    // save the new map size
    editor.map_w = newWidth;
    editor.map_h = newHeight;
    
    // clear and initialize the map
    for(int y = 0; y < GRID_SIZE; y++) {
        for(int x = 0; x < GRID_SIZE; x++) {
            editor.cells[y][x] = (x == 0 || y == 0 || x == editor.map_w-1 || y == editor.map_h-1) ? '1' : '0';
        }
    }
    
    center_grid();
}

void apply_size_change() {
    int newWidth = atoi(editor.sizeInputs[0].text);
    int newHeight = atoi(editor.sizeInputs[1].text);
    
    // validate the input values (ensure they're not empty)
    if (newWidth <= 0) newWidth = 20;
    if (newHeight <= 0) newHeight = 15;
    
    // don't show confirmation if values haven't changed
    if (newWidth == editor.map_w && newHeight == editor.map_h) {
        return;
    }
    
    // show confirmation dialog
    sprintf(editor.confirmDialog.message, "Change map size to %dx%d? All progress will be lost.", 
            newWidth, newHeight);
    editor.confirmDialog.active = 1;
    editor.confirmDialog.onConfirm = change_map_size;
    editor.confirmDialog.userData = NULL;
}

void draw_grid() {
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    
    float zoomed_cell_size = CELL_SIZE * editor.zoom_factor;

    // vertical lines
    for(int x=0; x<=editor.map_w; x++) { 
        SDL_RenderDrawLine(renderer, 
            x * zoomed_cell_size + editor.pan_offset.x,
            editor.pan_offset.y,
            x * zoomed_cell_size + editor.pan_offset.x,
            editor.map_h * zoomed_cell_size + editor.pan_offset.y
        );
    }

    // horizontal lines
    for(int y=0; y<=editor.map_h; y++) { 
        SDL_RenderDrawLine(renderer, 
            editor.pan_offset.x,
            y * zoomed_cell_size + editor.pan_offset.y,
            editor.map_w * zoomed_cell_size + editor.pan_offset.x,
            y * zoomed_cell_size + editor.pan_offset.y
        );
    }
}

void draw_blocks() {
    float zoomed_cell_size = CELL_SIZE * editor.zoom_factor;
    
    for(int y=0; y<editor.map_h; y++) {
        for(int x=0; x<editor.map_w; x++) {
            if(editor.cells[y][x] != '0') {
                SDL_Rect cell = {
                    x * zoomed_cell_size + editor.pan_offset.x + 1,
                    y * zoomed_cell_size + editor.pan_offset.y + 1,
                    zoomed_cell_size - 2,
                    zoomed_cell_size - 2
                };

                switch(editor.cells[y][x]) {
                    case '0': 
                        SDL_SetRenderDrawColor(renderer, 18, 18, 18, 255);
                        break;
                    case '1': 
                        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
                        break;
                    case '2': 
                        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                        break;
                    case '3': 
                        SDL_SetRenderDrawColor(renderer, 100, 100, 200, 255);
                        break;
                }
                SDL_RenderFillRect(renderer, &cell);
            }
        }
    }
}

int get_cursor_pos_from_mouse(UITextInput* input, int mouseX) {
    if (font == NULL) return 0;
    
    int relativeX = mouseX - input->rect.x - 5; 
    if (relativeX <= 0) return 0;
    
    // try each possible cursor position to find which one is closest to mouse
    for (int pos = 0; pos <= strlen(input->text); pos++) {
        char tempText[MAX_TEXT_INPUT_LENGTH + 1] = {0};
        strncpy(tempText, input->text, pos);
        tempText[pos] = '\0';
        
        SDL_Color textColor = {255, 255, 255, 255}; // added color definition
        SDL_Surface* tempSurface = TTF_RenderText_Blended(font, tempText, textColor);
        if (tempSurface != NULL) {
            int textWidth = tempSurface->w;
            SDL_FreeSurface(tempSurface);
            
            // if mouse is before this position, use previous position
            if (relativeX <= textWidth) {
                return pos;
            }
        }
    }
    
    return strlen(input->text); // default to end of text
}

void draw_text_input(UITextInput* input) {
    // draw the input field background
    if (input->active) {
        SDL_SetRenderDrawColor(renderer, 80, 80, 100, 255); // active color
    } else {
        SDL_SetRenderDrawColor(renderer, 60, 60, 80, 255); // inactive color
    }
    SDL_RenderFillRect(renderer, &input->rect);
    
    // draw the border
    if (input->active) {
        SDL_SetRenderDrawColor(renderer, 120, 120, 200, 255); // active border
    } else {
        SDL_SetRenderDrawColor(renderer, 80, 80, 100, 255); // inactive border
    }
    SDL_RenderDrawRect(renderer, &input->rect);
    
    // draw the text and selection if present
    if (font != NULL && input->text[0] != '\0') {
        SDL_Color textColor = {255, 255, 255, 255};
        
        // handle selection
        if (input->active && input->selectionStart != -1 && input->selectionEnd != -1) {
            int start = input->selectionStart < input->selectionEnd ? input->selectionStart : input->selectionEnd;
            int end = input->selectionStart < input->selectionEnd ? input->selectionEnd : input->selectionStart;
            
            // text before selection
            if (start > 0) {
                char beforeText[MAX_TEXT_INPUT_LENGTH + 1] = {0};
                strncpy(beforeText, input->text, start);
                beforeText[start] = '\0';
                
                SDL_Surface* beforeSurface = TTF_RenderText_Blended(font, beforeText, textColor);
                if (beforeSurface != NULL) {
                    SDL_Texture* beforeTexture = SDL_CreateTextureFromSurface(renderer, beforeSurface);
                    if (beforeTexture != NULL) {
                        SDL_Rect beforeRect = {
                            input->rect.x + 5,
                            input->rect.y + (input->rect.h - beforeSurface->h) / 2,
                            beforeSurface->w,
                            beforeSurface->h
                        };
                        SDL_RenderCopy(renderer, beforeTexture, NULL, &beforeRect);
                        SDL_DestroyTexture(beforeTexture);
                    }
                    SDL_FreeSurface(beforeSurface);
                }
            }
            
            // selected text
            char selectedText[MAX_TEXT_INPUT_LENGTH + 1] = {0};
            strncpy(selectedText, input->text + start, end - start);
            selectedText[end - start] = '\0';
            
            SDL_Surface* selectedSurface = TTF_RenderText_Blended(font, selectedText, textColor);
            if (selectedSurface != NULL) {
                int xPos = input->rect.x + 5;
                if (start > 0) {
                    char beforeText[MAX_TEXT_INPUT_LENGTH + 1] = {0};
                    strncpy(beforeText, input->text, start);
                    beforeText[start] = '\0';
                    
                    SDL_Surface* beforeSurface = TTF_RenderText_Solid(font, beforeText, textColor);
                    if (beforeSurface != NULL) {
                        xPos += beforeSurface->w;
                        SDL_FreeSurface(beforeSurface);
                    }
                }
                
                // draw selection background
                SDL_Rect selectionRect = {
                    xPos,
                    input->rect.y + (input->rect.h - selectedSurface->h) / 2,
                    selectedSurface->w,
                    selectedSurface->h
                };
                SDL_SetRenderDrawColor(renderer, 100, 100, 180, 255);
                SDL_RenderFillRect(renderer, &selectionRect);
                
                // draw selected text
                SDL_Texture* selectedTexture = SDL_CreateTextureFromSurface(renderer, selectedSurface);
                if (selectedTexture != NULL) {
                    SDL_RenderCopy(renderer, selectedTexture, NULL, &selectionRect);
                    SDL_DestroyTexture(selectedTexture);
                }
                SDL_FreeSurface(selectedSurface);
            }
            
            // text after selection
            if (end < strlen(input->text)) {
                char afterText[MAX_TEXT_INPUT_LENGTH + 1] = {0};
                strcpy(afterText, input->text + end);
                
                SDL_Surface* afterSurface = TTF_RenderText_Blended(font, afterText, textColor); // added missing textColor parameter
                if (afterSurface != NULL) {
                    int xPos = input->rect.x + 5;
                    char beforeEndText[MAX_TEXT_INPUT_LENGTH + 1] = {0};
                    strncpy(beforeEndText, input->text, end);
                    beforeEndText[end] = '\0';
                    
                    SDL_Surface* beforeEndSurface = TTF_RenderText_Solid(font, beforeEndText, textColor);
                    if (beforeEndSurface != NULL) {
                        xPos += beforeEndSurface->w;
                        SDL_FreeSurface(beforeEndSurface);
                    }
                    
                    SDL_Texture* afterTexture = SDL_CreateTextureFromSurface(renderer, afterSurface);
                    if (afterTexture != NULL) {
                        SDL_Rect afterRect = {
                            xPos,
                            input->rect.y + (input->rect.h - afterSurface->h) / 2,
                            afterSurface->w,
                            afterSurface->h
                        };
                        SDL_RenderCopy(renderer, afterTexture, NULL, &afterRect);
                        SDL_DestroyTexture(afterTexture);
                    }
                    SDL_FreeSurface(afterSurface);
                }
            }
        } else {
            // regular text rendering without selection
            SDL_Surface* textSurface = TTF_RenderText_Blended(font, input->text, textColor);
            if (textSurface != NULL) {
                SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
                if (textTexture != NULL) {
                    SDL_Rect textRect = {
                        input->rect.x + 5,
                        input->rect.y + (input->rect.h - textSurface->h) / 2,
                        textSurface->w,
                        textSurface->h
                    };
                    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
                    SDL_DestroyTexture(textTexture);
                }
                SDL_FreeSurface(textSurface);
            }
        }
    }
    
    // draw cursor when active and not selecting text
    if (input->active && (input->selectionStart == -1 || input->selectionEnd == -1)) {
        // get the width of the text up to cursor position
        int cursorX = input->rect.x + 5;
        
        if (input->cursorPosition > 0 && font != NULL) {
            // get width of text before cursor
            char tempText[MAX_TEXT_INPUT_LENGTH + 1] = {0};
            strncpy(tempText, input->text, input->cursorPosition);
            tempText[input->cursorPosition] = '\0';
            
            SDL_Color textColor = {255, 255, 255, 255};
            SDL_Surface* tempSurface = TTF_RenderText_Blended(font, tempText, textColor);
            if (tempSurface != NULL) {
                cursorX += tempSurface->w;
                SDL_FreeSurface(tempSurface);
            }
        }
        
        // draw the cursor line
        int cursorHeight = input->rect.h - 10; // slightly shorter than input height
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawLine(
            renderer,
            cursorX,
            input->rect.y + 5,
            cursorX,
            input->rect.y + cursorHeight
        );
    }
}

int validate_digit_input(const char* text, char c) {
    // only allow digits
    if (c < '0' || c > '9') {
        return 0;
    }
    
    // check if adding this character would exceed max length
    if (strlen(text) >= MAX_TEXT_INPUT_LENGTH) {
        return 0;
    }
    
    return 1;
}

void handle_text_input(UITextInput* input, SDL_Event* e) {
    if (!input->active) return;
    
    // handle text input
    if (e->type == SDL_TEXTINPUT) {
        // clear any selection when typing
        if (input->selectionStart != -1 && input->selectionEnd != -1) {
            int start = input->selectionStart < input->selectionEnd ? input->selectionStart : input->selectionEnd;
            int end = input->selectionStart < input->selectionEnd ? input->selectionEnd : input->selectionStart;
            
            // remove selected text first
            memmove(
                &input->text[start],
                &input->text[end],
                strlen(&input->text[end]) + 1
            );
            input->cursorPosition = start;
            input->selectionStart = -1;
            input->selectionEnd = -1;
        }
        
        // only accept digits and enforce length limit
        char c = e->text.text[0];
        if (validate_digit_input(input->text, c)) {
            // insert at cursor position
            if (strlen(input->text) < input->maxLength) {
                memmove(
                    &input->text[input->cursorPosition + 1],
                    &input->text[input->cursorPosition],
                    strlen(&input->text[input->cursorPosition]) + 1
                );
                input->text[input->cursorPosition] = c;
                input->cursorPosition++;
            }
        }
    }
    else if (e->type == SDL_KEYDOWN) {
        switch (e->key.keysym.sym) {
            case SDLK_BACKSPACE:
                if (input->selectionStart != -1 && input->selectionEnd != -1) {
                    // delete selection
                    int start = input->selectionStart < input->selectionEnd ? input->selectionStart : input->selectionEnd;
                    int end = input->selectionStart < input->selectionEnd ? input->selectionEnd : input->selectionStart;
                    
                    memmove(
                        &input->text[start],
                        &input->text[end],
                        strlen(&input->text[end]) + 1
                    );
                    input->cursorPosition = start;
                    input->selectionStart = -1;
                    input->selectionEnd = -1;
                } else if (input->cursorPosition > 0) {
                    // regular backspace
                    memmove(
                        &input->text[input->cursorPosition - 1], 
                        &input->text[input->cursorPosition],
                        strlen(&input->text[input->cursorPosition]) + 1
                    );
                    input->cursorPosition--;
                }
                break;
                
            case SDLK_DELETE:
                if (input->selectionStart != -1 && input->selectionEnd != -1) {
                    // delete selection
                    int start = input->selectionStart < input->selectionEnd ? input->selectionStart : input->selectionEnd;
                    int end = input->selectionStart < input->selectionEnd ? input->selectionEnd : input->selectionStart;
                    
                    memmove(
                        &input->text[start],
                        &input->text[end],
                        strlen(&input->text[end]) + 1
                    );
                    input->cursorPosition = start;
                    input->selectionStart = -1;
                    input->selectionEnd = -1;
                } else if (input->cursorPosition < strlen(input->text)) {
                    // regular delete
                    memmove(
                        &input->text[input->cursorPosition], 
                        &input->text[input->cursorPosition + 1],
                        strlen(&input->text[input->cursorPosition + 1]) + 1
                    );
                }
                break;
                
            case SDLK_LEFT:
                if (input->cursorPosition > 0) {
                    input->cursorPosition--;
                }
                break;
                
            case SDLK_RIGHT:
                if (input->cursorPosition < strlen(input->text)) {
                    input->cursorPosition++;
                }
                break;
                
            case SDLK_HOME:
                input->cursorPosition = 0;
                break;
                
            case SDLK_END:
                input->cursorPosition = strlen(input->text);
                break;
                
            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                // deactivate input and apply
                input->active = 0;
                break;
                
            case SDLK_a:
                if (e->key.keysym.mod & KMOD_CTRL) {
                    // select all text
                    input->selectionStart = 0;
                    input->selectionEnd = strlen(input->text);
                    input->cursorPosition = input->selectionEnd;
                }
                break;
        }
    }
}

void draw_dialog(UIDialog* dialog) {
    if (!dialog->active) return;
    
    // semi-transparent overlay
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 192);
    SDL_Rect overlay = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
    SDL_RenderFillRect(renderer, &overlay);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    
    // dialog background
    SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
    SDL_RenderFillRect(renderer, &dialog->rect);
    
    // dialog border
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderDrawRect(renderer, &dialog->rect);
    
    // draw message text
    if (font != NULL) {
        SDL_Color textColor = {255, 255, 255, 255};
        SDL_Surface* textSurface = TTF_RenderText_Blended_Wrapped(font, dialog->message, textColor, DIALOG_WIDTH - 40);
        if (textSurface != NULL) {
            SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
            if (textTexture != NULL) {
                SDL_Rect textRect = {
                    dialog->rect.x + (dialog->rect.w - textSurface->w) / 2,
                    dialog->rect.y + 20,
                    textSurface->w,
                    textSurface->h
                };
                SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
                SDL_DestroyTexture(textTexture);
            }
            SDL_FreeSurface(textSurface);
        }
    }
    
    // DRAW BUTTONS
    // ok button
    SDL_SetRenderDrawColor(renderer, 80, 150, 80, 255);
    SDL_RenderFillRect(renderer, &dialog->okButton);
    SDL_SetRenderDrawColor(renderer, 100, 180, 100, 255);
    SDL_RenderDrawRect(renderer, &dialog->okButton);
    
    // cancel button
    SDL_SetRenderDrawColor(renderer, 150, 80, 80, 255);
    SDL_RenderFillRect(renderer, &dialog->cancelButton);
    SDL_SetRenderDrawColor(renderer, 180, 100, 100, 255);
    SDL_RenderDrawRect(renderer, &dialog->cancelButton);
    
    // button text
    if (font != NULL) {
        // ok button text
        SDL_Color textColor = {255, 255, 255, 255};
        SDL_Surface* okSurface = TTF_RenderText_Blended(font, "OK", textColor);
        if (okSurface != NULL) {
            SDL_Texture* okTexture = SDL_CreateTextureFromSurface(renderer, okSurface);
            if (okTexture != NULL) {
                SDL_Rect textRect = {
                    dialog->okButton.x + (dialog->okButton.w - okSurface->w) / 2,
                    dialog->okButton.y + (dialog->okButton.h - okSurface->h) / 2,
                    okSurface->w,
                    okSurface->h
                };
                SDL_RenderCopy(renderer, okTexture, NULL, &textRect);
                SDL_DestroyTexture(okTexture);
            }
            SDL_FreeSurface(okSurface);
        }
        
        // cancel button text
        SDL_Surface* cancelSurface = TTF_RenderText_Blended(font, "Cancel", textColor);
        if (cancelSurface != NULL) {
            SDL_Texture* cancelTexture = SDL_CreateTextureFromSurface(renderer, cancelSurface);
            if (cancelTexture != NULL) {
                SDL_Rect textRect = {
                    dialog->cancelButton.x + (dialog->cancelButton.w - cancelSurface->w) / 2,
                    dialog->cancelButton.y + (dialog->cancelButton.h - cancelSurface->h) / 2,
                    cancelSurface->w,
                    cancelSurface->h
                };
                SDL_RenderCopy(renderer, cancelTexture, NULL, &textRect);
                SDL_DestroyTexture(cancelTexture);
            }
            SDL_FreeSurface(cancelSurface);
        }
    }
}

void draw_ui() {
    // draw the tabs
    for (int i = 0; i < 4; i++) {
        // draw tab background
        if (editor.current_tab == i) {
            // active tab brighter
            SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
        } else {
            // inactive tab darker
            SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
        }
        SDL_RenderFillRect(renderer, &tabs[i].rect);

        // draw tab border
        SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
        SDL_RenderDrawRect(renderer, &tabs[i].rect);

        // draw tab label
        if (font != NULL) {
            SDL_Color textColor = {255, 255, 255, 255};
            SDL_Surface* textSurface = TTF_RenderText_Blended(font, tabs[i].label, textColor);
            if (textSurface != NULL) {
                SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
                if (textTexture != NULL) {
                    SDL_Rect textRect = {
                        tabs[i].rect.x + (tabs[i].rect.w - textSurface->w) / 2,
                        tabs[i].rect.y + (tabs[i].rect.h - textSurface->h) / 2,
                        textSurface->w,
                        textSurface->h
                    };
                    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
                    SDL_DestroyTexture(textTexture);
                }
                SDL_FreeSurface(textSurface);
            }
        }
    }

    // tool panel bg
    SDL_Rect panel = {WINDOW_WIDTH-UI_PANEL_WIDTH, 0, UI_PANEL_WIDTH, WINDOW_HEIGHT};
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderFillRect(renderer, &panel);

    // instructions
    SDL_Rect help_rect = {WINDOW_WIDTH-UI_PANEL_WIDTH+10, 10, UI_PANEL_WIDTH-20, 30};
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    SDL_RenderFillRect(renderer, &help_rect);

    // only show map size controls if on the painter tab
    if (editor.current_tab == TAB_PAINTER) {
        // draw "map size" label at the top
        if (font != NULL) {
            SDL_Color textColor = {255, 255, 255, 255};
            SDL_Surface* mapSizeSurface = TTF_RenderText_Blended(font, "MAP SIZE:", textColor);
            if (mapSizeSurface != NULL) {
                SDL_Texture* mapSizeTexture = SDL_CreateTextureFromSurface(renderer, mapSizeSurface);
                if (mapSizeTexture != NULL) {
                    SDL_Rect labelRect = {
                        WINDOW_WIDTH - UI_PANEL_WIDTH + 10,
                        editor.sizeInputs[0].rect.y - 20,
                        mapSizeSurface->w,
                        mapSizeSurface->h
                    };
                    SDL_RenderCopy(renderer, mapSizeTexture, NULL, &labelRect);
                    SDL_DestroyTexture(mapSizeTexture);
                }
                SDL_FreeSurface(mapSizeSurface);
            }
        }

        // draw the width and height labels
        if (font != NULL) {
            SDL_Color textColor = {255, 255, 255, 255};
            
            // draw the width label properly positioned
            SDL_Surface* wLabelSurface = TTF_RenderText_Blended(font, "W:", textColor);
            if (wLabelSurface != NULL) {
                SDL_Texture* wLabelTexture = SDL_CreateTextureFromSurface(renderer, wLabelSurface);
                if (wLabelTexture != NULL) {
                    SDL_Rect labelRect = {
                        editor.sizeInputs[0].rect.x - 20,
                        editor.sizeInputs[0].rect.y + (editor.sizeInputs[0].rect.h - wLabelSurface->h) / 2,
                        wLabelSurface->w,
                        wLabelSurface->h
                    };
                    SDL_RenderCopy(renderer, wLabelTexture, NULL, &labelRect);
                    SDL_DestroyTexture(wLabelTexture);
                }
                SDL_FreeSurface(wLabelSurface);
            }
            
            // draw the height label properly positioned
            SDL_Surface* hLabelSurface = TTF_RenderText_Blended(font, "H:", textColor);
            if (hLabelSurface != NULL) {
                SDL_Texture* hLabelTexture = SDL_CreateTextureFromSurface(renderer, hLabelSurface);
                if (hLabelTexture != NULL) {
                    SDL_Rect labelRect = {
                        editor.sizeInputs[1].rect.x - 20,
                        editor.sizeInputs[1].rect.y + (editor.sizeInputs[1].rect.h - hLabelSurface->h) / 2,
                        hLabelSurface->w,
                        hLabelSurface->h
                    };
                    SDL_RenderCopy(renderer, hLabelTexture, NULL, &labelRect);
                    SDL_DestroyTexture(hLabelTexture);
                }
                SDL_FreeSurface(hLabelSurface);
            }
        }

        // draw the text input fields
        draw_text_input(&editor.sizeInputs[0]);
        draw_text_input(&editor.sizeInputs[1]);
        
        // apply button: draw
        SDL_SetRenderDrawColor(renderer,
            editor.applyButton.color.r,
            editor.applyButton.color.g,
            editor.applyButton.color.b,
            255
        );
        SDL_RenderFillRect(renderer, &editor.applyButton.rect);

        // apply button: border
        SDL_SetRenderDrawColor(renderer, 100, 140, 200, 255);
        SDL_RenderDrawRect(renderer, &editor.applyButton.rect);

        // apply button: text
        if (font != NULL) {
            SDL_Color textColor = {255, 255, 255, 255};
            SDL_Surface* textSurface = TTF_RenderText_Blended(font, editor.applyButton.label, textColor);
            if (textSurface != NULL) {
                SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
                
                if (textTexture != NULL) {
                    SDL_Rect textRect = {
                      editor.applyButton.rect.x + (editor.applyButton.rect.w - textSurface->w) / 2,
                      editor.applyButton.rect.y + (editor.applyButton.rect.h - textSurface->h) / 2,
                      textSurface->w,
                      textSurface->h
                };
                SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
                SDL_DestroyTexture(textTexture);
                }
                SDL_FreeSurface(textSurface);
            }
        }

        // draw divider
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderDrawLine(
            renderer,
            WINDOW_WIDTH - UI_PANEL_WIDTH + 10,
            editor.applyButton.rect.y + editor.applyButton.rect.h + 10,
            WINDOW_WIDTH - 10,
            editor.applyButton.rect.y + editor.applyButton.rect.h + 10
        );

        // draw tools with labels
        for(int i=0; i<5; i++) {
            SDL_Color c = tools[i].color;
            SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
            SDL_RenderFillRect(renderer, &tools[i].rect);

            // border
            SDL_SetRenderDrawColor(renderer, 100, 140, 200, 255);
            SDL_RenderDrawRect(renderer, &editor.applyButton.rect);

            // button txt
            if (font != NULL) {
                SDL_Color textColor = {255, 255, 255, 255};
                SDL_Surface* textSurface = TTF_RenderText_Blended(font, editor.applyButton.label, textColor);
                if (textSurface != NULL) {
                    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
                    if (textTexture != NULL) {
                       SDL_Rect textRect = {
                           editor.applyButton.rect.x + (editor.applyButton.rect.w - textSurface->w) / 2,
                           editor.applyButton.rect.y + (editor.applyButton.rect.h - textSurface->h) / 2,
                                   textSurface->w,
                                   textSurface->h
                };
                SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
                SDL_DestroyTexture(textTexture);
                    }
                    SDL_FreeSurface(textSurface);
                }
            }

            // selection border
            if(editor.selected_tool == i) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderDrawRect(renderer, &tools[i].rect);
            }

            // draw label text
            if (font != NULL) {
                // determine text color
                SDL_Color textColor;
                if (i == TOOL_LIGHT && is_light_color(tools[i].color)) {
                    textColor = (SDL_Color){0, 0, 0, 255}; // Black text on light background
                } else {
                    textColor = (SDL_Color){255, 255, 255, 255}; // White text otherwise
                }
                
                SDL_Surface* textSurface = TTF_RenderText_Blended(font, tools[i].label, textColor);
                if (textSurface != NULL) {
                    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
                    if (textTexture != NULL) {
                        SDL_Rect textRect = {
                            tools[i].rect.x + (tools[i].rect.w - textSurface->w) / 2, // center text horizontally
                            tools[i].rect.y + (tools[i].rect.h - textSurface->h) / 2,
                            textSurface->w,
                            textSurface->h
                        };
                        SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
                        SDL_DestroyTexture(textTexture);
                    }
                    SDL_FreeSurface(textSurface);
                } else {
                    // fallback
                    // create a centered placeholder rectangle
                    int rectWidth = 20;
                    SDL_Rect textRect = {
                        tools[i].rect.x + (tools[i].rect.w - rectWidth) / 2,
                        tools[i].rect.y + (tools[i].rect.h - 20) / 2,
                        rectWidth,
                        20
                    };
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    SDL_RenderDrawRect(renderer, &textRect);
                }
            }
        }
    }

    switch (editor.current_tab) {
        case TAB_RENDERING:
            // placeholder for rendering tab
            {
                SDL_Rect renderMsg = {WINDOW_WIDTH-UI_PANEL_WIDTH+10, 60, UI_PANEL_WIDTH-20, 30};
                SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
                SDL_RenderFillRect(renderer, &renderMsg);
            }
            break;
            
        case TAB_TEXTURE:
            // placeholder for texture tab
            {
                SDL_Rect textureMsg = {WINDOW_WIDTH-UI_PANEL_WIDTH+10, 60, UI_PANEL_WIDTH-20, 30};
                SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
                SDL_RenderFillRect(renderer, &textureMsg);
            }
            break;
            
        case TAB_COMPILE:
            // draw compile buttons
            draw_compile_buttons();
            break;
    }
    
    // instruction text at bottom
    SDL_Rect instructions = {WINDOW_WIDTH-UI_PANEL_WIDTH+10, WINDOW_HEIGHT-100, UI_PANEL_WIDTH-20, 80};
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    SDL_RenderFillRect(renderer, &instructions);

    // draw the moon engine title/logo in the right panel
    draw_panel_logo();

    // draw dialog if active
    draw_dialog(&editor.confirmDialog);

    // draw color picker if active
    draw_color_picker(&editor.colorPicker);
}

void draw_console() {
    // draw console background
    SDL_Rect consoleRect = {0, WINDOW_HEIGHT - CONSOLE_HEIGHT, WINDOW_WIDTH - UI_PANEL_WIDTH, CONSOLE_HEIGHT};
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);  // darker background for console
    SDL_RenderFillRect(renderer, &consoleRect);
    
    // draw console border
    SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
    SDL_RenderDrawRect(renderer, &consoleRect);
    
    // draw console header
    SDL_Rect headerRect = {consoleRect.x, consoleRect.y, consoleRect.w, 20};
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    SDL_RenderFillRect(renderer, &headerRect);
    
    // draw "console" text in header
    if (font != NULL) {
        SDL_Color textColor = {180, 180, 180, 255};
        SDL_Surface* textSurface = TTF_RenderText_Blended(font, "Debug Console", textColor);
        if (textSurface != NULL) {
            SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
            if (textTexture != NULL) {
                SDL_Rect textRect = {
                    headerRect.x + 5,
                    headerRect.y + (headerRect.h - textSurface->h) / 2,
                    textSurface->w,
                    textSurface->h
                };
                SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
                SDL_DestroyTexture(textTexture);
            }
            SDL_FreeSurface(textSurface);
        }
    }
    
    // calculate visible area for console text
    SDL_Rect textArea = {
        consoleRect.x + 5,
        consoleRect.y + headerRect.h + 5,
        consoleRect.w - 10,
        consoleRect.h - headerRect.h - 10
    };
    
    // determine how many lines can be displayed
    int line_height = 15;
    int visible_lines = textArea.h / line_height;
    
    // draw console text - starting from scroll_offset
    if (font != NULL) {
        SDL_Color textColor = {200, 200, 200, 255};
        int start_line = editor.console.scroll_offset;
        int end_line = start_line + visible_lines;
        if (end_line > editor.console.line_count) {
            end_line = editor.console.line_count;
        }
        
        for (int i = start_line; i < end_line; i++) {
            SDL_Surface* textSurface = TTF_RenderText_Blended(font, editor.console.lines[i], textColor);
            if (textSurface != NULL) {
                SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
                if (textTexture != NULL) {
                    SDL_Rect textRect = {
                        textArea.x,
                        textArea.y + (i - start_line) * line_height,
                        textSurface->w,
                        textSurface->h
                    };
                    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
                    SDL_DestroyTexture(textTexture);
                }
                SDL_FreeSurface(textSurface);
            }
        }
    }
    
    // draw scroll indicators if needed
    if (editor.console.scroll_offset > 0) {
        // draw "more above" indicator
        SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
        for (int i = 0; i < 3; i++) {
            SDL_RenderDrawLine(renderer, 
                consoleRect.x + consoleRect.w - 15, consoleRect.y + headerRect.h + 10 + i*3,
                consoleRect.x + consoleRect.w - 5, consoleRect.y + headerRect.h + 10 + i*3
            );
        }
    }
    
    if (editor.console.line_count > editor.console.scroll_offset + visible_lines) {
        // draw "more below" indicator
        SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
        for (int i = 0; i < 3; i++) {
            SDL_RenderDrawLine(renderer, 
                consoleRect.x + consoleRect.w - 15, consoleRect.y + consoleRect.h - 10 - i*3,
                consoleRect.x + consoleRect.w - 5, consoleRect.y + consoleRect.h - 10 - i*3
            );
        }
    }
}

void append_to_console(const char* message) {
    // don't add empty messages
    if (!message || message[0] == '\0')
        return;
    
    // if at max capacity, shift everything up one line 
    if (editor.console.line_count >= MAX_CONSOLE_LINES) {
        for (int i = 0; i < MAX_CONSOLE_LINES - 1; i++) {
            strcpy(editor.console.lines[i], editor.console.lines[i + 1]);
        }
        editor.console.line_count = MAX_CONSOLE_LINES - 1;
    }
    
    // add the new message
    strncpy(editor.console.lines[editor.console.line_count], message, MAX_CONSOLE_LINE_LENGTH - 1);
    editor.console.lines[editor.console.line_count][MAX_CONSOLE_LINE_LENGTH - 1] = '\0'; // ensure null-termination
    editor.console.line_count++;
    
    // auto-scroll to bottom
    editor.console.scroll_offset = editor.console.line_count > 0 ? editor.console.line_count - 1 : 0;
}

void clear_console() {
    editor.console.line_count = 0;
    editor.console.scroll_offset = 0;
}

void draw_panel_logo() {
    if (font != NULL) {
        // create a larger font for the logo
        TTF_Font* logoFont = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 18);
        if (logoFont == NULL) {
            // fallback to regular font if bold version not available
            logoFont = font;
        }

        SDL_Color textColor = {180, 180, 180, 255}; // light gray
        SDL_Surface* textSurface = TTF_RenderText_Blended(logoFont, "MOON Engine", textColor);

        if (textSurface != NULL) {
            SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface); // fixed 'renderer'
            if (textTexture != NULL) {
                // position in the instruction box at the bottom of the right panel
                // using the same dimensions as the instructions rect in draw_ui()
                SDL_Rect instructionsRect = {
                    WINDOW_WIDTH - UI_PANEL_WIDTH + 10, 
                    WINDOW_HEIGHT - 100, 
                    UI_PANEL_WIDTH - 20, 
                    80
                };
                
                SDL_Rect textRect = {
                    instructionsRect.x + (instructionsRect.w - textSurface->w) / 2,  // centered horizontally in the box
                    instructionsRect.y + (instructionsRect.h - textSurface->h) / 2,  // centered vertically in the box
                    textSurface->w,
                    textSurface->h
                };
                SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
                SDL_DestroyTexture(textTexture);
            }
            SDL_FreeSurface(textSurface);
        }

        // clean up the logo font if it's different from the regular font
        if (logoFont != NULL && logoFont != font) {
            TTF_CloseFont(logoFont);
        }
    }
}

// handle dialog
void handle_dialog(UIDialog* dialog, int mx, int my) {
    if (!dialog->active) return;
    
    // check for ok button click
    if (SDL_PointInRect(&(SDL_Point){mx, my}, &dialog->okButton)) {
        if (dialog->onConfirm != NULL) {
            dialog->onConfirm(dialog->userData);
        }
        dialog->active = 0;
    }
    
    // check for cancel button click
    if (SDL_PointInRect(&(SDL_Point){mx, my}, &dialog->cancelButton)) {
        dialog->active = 0;
    }
}

void handle_mouse(SDL_Event* e) {
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    
    float zoomed_cell_size = CELL_SIZE * editor.zoom_factor;

    if(e->type == SDL_MOUSEBUTTONDOWN) {
        if(e->button.button == SDL_BUTTON_LEFT) {
            // check for dialog interaction first
            if (editor.confirmDialog.active) {
                handle_dialog(&editor.confirmDialog, mx, my);
                return;
            }
            
            // check text input fields
            int inputActivated = 0;
            for (int i = 0; i < MAX_INPUT_FIELDS; i++) {
                if (SDL_PointInRect(&(SDL_Point){mx, my}, &editor.sizeInputs[i].rect)) {
                    // activate this input, deactivate others
                    for (int j = 0; j < MAX_INPUT_FIELDS; j++) {
                        editor.sizeInputs[j].active = (i == j);
                    }
                    inputActivated = 1;
                    
                    // set cursor position based on mouse click position
                    if (editor.sizeInputs[i].active) {
                        editor.sizeInputs[i].cursorPosition = get_cursor_pos_from_mouse(&editor.sizeInputs[i], mx);
                        editor.sizeInputs[i].selectionStart = editor.sizeInputs[i].cursorPosition;
                        editor.sizeInputs[i].selectionEnd = editor.sizeInputs[i].cursorPosition;
                    }
                    
                    // start text input mode if not already active
                    if (!SDL_IsTextInputActive()) {
                        SDL_StartTextInput();
                    }
                    
                    return;
                }
            }
            
            // deactivate all inputs if clicking elsewhere
            if (!inputActivated) {
                for (int i = 0; i < MAX_INPUT_FIELDS; i++) {
                    if (editor.sizeInputs[i].active) {
                        editor.sizeInputs[i].active = 0;
                        if (SDL_IsTextInputActive()) {
                            SDL_StopTextInput();
                        }
                    }
                }
            }
            
            // check apply button
            if (SDL_PointInRect(&(SDL_Point){mx, my}, &editor.applyButton.rect)) {
                apply_size_change();
                return;
            }
            
            // check ui first - check if tabs were clicked
            for(int i=0; i<4; i++) {
                if(SDL_PointInRect(&(SDL_Point){mx, my}, &tabs[i].rect)) {
                    editor.current_tab = i;
                    return;
                }
            }

            // check if tools were clicked (only in painter tab)
            if(editor.current_tab == TAB_PAINTER) {
                for(int i=0; i<5; i++) {
                    if(SDL_PointInRect(&(SDL_Point){mx, my}, &tools[i].rect)) {
                        editor.selected_tool = i;
                        
                        // Check if COLOR button was clicked and open the color picker
                        if (i == TOOL_COLOR_PICKER) {
                            editor.colorPicker.active = 1;
                            editor.colorPicker.currentColor = editor.lightColor;
                            editor.colorPicker.originalColor = editor.lightColor;
                        }
                        return;
                    }
                }
            }

            // check if compile buttons were clicked when in compile tab
            if (editor.current_tab == TAB_COMPILE) {
                for(int i=0; i<MAX_COMPILE_BUTTONS; i++) {
                    if(SDL_PointInRect(&(SDL_Point){mx, my}, &compileButtons[i].rect)) {
                        // handle button clicks
                        switch(i) {
                            case 0: // compile to map
                                if (check_file_exists(MAP_FILE_PATH)) {
                                    // ask for confirmation
                                    strcpy(editor.confirmDialog.message, "Map file already exists. Are you sure you want to overwrite?");
                                    editor.confirmDialog.active = 1;
                                    editor.confirmDialog.onConfirm = save_map_to_file;
                                    editor.confirmDialog.userData = NULL;
                                } else {
                                    // no existing file, just save
                                    save_map_to_file();
                                }
                                return;
                                
                            case 1: // load from map
                                if (check_file_exists(MAP_FILE_PATH)) {
                                    // ask for confirmation
                                    strcpy(editor.confirmDialog.message, "Load from map? This will clear your current work.");
                                    editor.confirmDialog.active = 1;
                                    editor.confirmDialog.onConfirm = load_map_from_file;
                                    editor.confirmDialog.userData = NULL;
                                } else {
                                    show_map_not_found_dialog();
                                }
                                return;
                                
                            case 2: // open map.txt
                                open_map_in_editor();
                                return;
                        }
                    }
                }
            }

            // map editing (NOW AVAILABLE ON ALL TABS)
            editor.mouse_down = 1;
            editor.light_placed = 0;
            SDL_Point grid_pos = {
                (mx - editor.pan_offset.x) / zoomed_cell_size,
                (my - editor.pan_offset.y) / zoomed_cell_size
            };

            if(grid_pos.x >= 0 && grid_pos.x < editor.map_w &&
               grid_pos.y >= 0 && grid_pos.y < editor.map_h) {
                
                // handle player placement (only one allowed)
                if (editor.selected_tool == TOOL_PLAYER) {
                    // if there's an existing player, remove it first
                    if (editor.player_exists) {
                        editor.cells[editor.player_pos.y][editor.player_pos.x] = '0';
                    }
                    
                    // place new player and update tracking
                    editor.cells[grid_pos.y][grid_pos.x] = '0' + TOOL_PLAYER;
                    editor.player_exists = 1;
                    editor.player_pos = grid_pos;
                }
                // handle light placement (one at a time)
                else if (editor.selected_tool == TOOL_LIGHT) {
                    editor.cells[grid_pos.y][grid_pos.x] = '0' + TOOL_LIGHT;
                    editor.light_placed = 1;
                }
                // handle other tools normally
                else {
                    editor.cells[grid_pos.y][grid_pos.x] = '0' + editor.selected_tool;
                }
            }
        }
        else if(e->button.button == SDL_BUTTON_RIGHT) {
            // check if right-clicking the light button
            if (SDL_PointInRect(&(SDL_Point){mx, my}, &tools[TOOL_LIGHT].rect)) {
                // open color picker
                editor.colorPicker.active = 1;
                editor.colorPicker.currentColor = editor.lightColor;
                editor.colorPicker.originalColor = editor.lightColor;
                return;
            }
            
            editor.panning = 1;
            editor.last_mouse.x = mx;
            editor.last_mouse.y = my;
        }
    }
    else if(e->type == SDL_MOUSEBUTTONUP) {
        // complete any text selection
        for (int i = 0; i < MAX_INPUT_FIELDS; i++) {
            if (editor.sizeInputs[i].active && editor.sizeInputs[i].selectionStart != -1) {
                editor.sizeInputs[i].selectionEnd = get_cursor_pos_from_mouse(&editor.sizeInputs[i], mx);
                // if click without drag, clear selection
                if (editor.sizeInputs[i].selectionStart == editor.sizeInputs[i].selectionEnd) {
                    editor.sizeInputs[i].selectionStart = -1;
                    editor.sizeInputs[i].selectionEnd = -1;
                }
            }
        }
        
        editor.mouse_down = 0;
        editor.panning = 0;
    }
    else if(e->type == SDL_MOUSEMOTION) {
        // handle text selection with mouse drag
        if (e->motion.state & SDL_BUTTON_LMASK) {
            for (int i = 0; i < MAX_INPUT_FIELDS; i++) {
                if (editor.sizeInputs[i].active && editor.sizeInputs[i].selectionStart != -1) {
                    int newPos = get_cursor_pos_from_mouse(&editor.sizeInputs[i], mx);
                    if (newPos != editor.sizeInputs[i].selectionEnd) {
                        editor.sizeInputs[i].selectionEnd = newPos;
                        editor.sizeInputs[i].cursorPosition = newPos;
                    }
                }
            }
        }
        
        if(editor.panning) {
            editor.pan_offset.x += mx - editor.last_mouse.x;
            editor.pan_offset.y += my - editor.last_mouse.y;
            editor.last_mouse.x = mx;
            editor.last_mouse.y = my;
        }
        else if(editor.mouse_down) {
            // only continue painting if not using the light tool
            if (editor.selected_tool != TOOL_LIGHT || !editor.light_placed) {
                SDL_Point grid_pos = {
                    (mx - editor.pan_offset.x) / zoomed_cell_size,
                    (my - editor.pan_offset.y) / zoomed_cell_size
                };

                if(grid_pos.x >= 0 && grid_pos.x < editor.map_w &&
                   grid_pos.y >= 0 && grid_pos.y < editor.map_h) {
                    
                    // handle player placement (only one allowed)
                    if (editor.selected_tool == TOOL_PLAYER) {
                        // only place if different from current position
                        if (!editor.player_exists || 
                            grid_pos.x != editor.player_pos.x || 
                            grid_pos.y != editor.player_pos.y) {
                            
                            // remove existing player
                            if (editor.player_exists) {
                                editor.cells[editor.player_pos.y][editor.player_pos.x] = '0';
                            }
                            
                            // place new player
                            editor.cells[grid_pos.y][grid_pos.x] = '0' + TOOL_PLAYER;
                            editor.player_exists = 1;
                            editor.player_pos = grid_pos;
                        }
                    }
                    // handle light placement (one at a time - skip during drag)
                    else if (editor.selected_tool == TOOL_LIGHT) {
                        // light placement is handled in mousebuttondown
                    }
                    // handle other tools normally
                    else {
                        editor.cells[grid_pos.y][grid_pos.x] = '0' + editor.selected_tool;
                    }
                }
            }
        }
    }
    
    // add handling for color picker events
    handle_color_picker(&editor.colorPicker, e);
}

void handle_mouse_wheel(SDL_Event* e) {
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    
    // check if mouse is over console
    SDL_Rect consoleRect = {0, WINDOW_HEIGHT - CONSOLE_HEIGHT, WINDOW_WIDTH - UI_PANEL_WIDTH, CONSOLE_HEIGHT};
    if (SDL_PointInRect(&(SDL_Point){mx, my}, &consoleRect)) {
        // handle console scrolling
        if (e->wheel.y > 0) {  // scroll up
            if (editor.console.scroll_offset > 0)
                editor.console.scroll_offset--;
        }
        else if (e->wheel.y < 0) {  // scroll down
            int line_height = 15; // approximate line height
            int visible_lines = (consoleRect.h - 20 - 10) / line_height; // 20 for header, 10 for padding
            
            if (editor.console.scroll_offset + visible_lines < editor.console.line_count)
                editor.console.scroll_offset++;
        }
        return;
    }
    
    // calculate the view center in world coordinates (before zoom)
    float center_x = (WINDOW_WIDTH / 2 - editor.pan_offset.x) / (CELL_SIZE * editor.zoom_factor);
    float center_y = (WINDOW_HEIGHT / 2 - editor.pan_offset.y) / (CELL_SIZE * editor.zoom_factor);
    
    // store old zoom factor
    float old_zoom = editor.zoom_factor;
    
    // apply zoom
    if (e->wheel.y > 0) {  // scroll up, zoom in
        editor.zoom_factor *= 1.1f;
        if (editor.zoom_factor > 3.0f) editor.zoom_factor = 3.0f;  // maximum zoom limit
    }
    else if (e->wheel.y < 0) {  // scroll down, zoom out
        editor.zoom_factor *= 0.9f;
        if (editor.zoom_factor < 0.3f) editor.zoom_factor = 0.3f;  // minimum zoom limit
    }
    
    // adjust pan_offset to keep the center point at the center of the view
    editor.pan_offset.x = WINDOW_WIDTH / 2 - center_x * CELL_SIZE * editor.zoom_factor;
    editor.pan_offset.y = WINDOW_HEIGHT / 2 - center_y * CELL_SIZE * editor.zoom_factor;
}

void init_compile_buttons() {
    int button_width = UI_PANEL_WIDTH - 20;
    int button_height = 40;
    int button_padding = 10;
    int button_y_spacing = 10;
    int button_start_y = 60;

    // compile to map button
    compileButtons[0].rect = (SDL_Rect){
        WINDOW_WIDTH-UI_PANEL_WIDTH+button_padding, 
        button_start_y,
        button_width, 
        button_height
    };
    strcpy(compileButtons[0].label, "COMPILE TO MAP");
    compileButtons[0].color = (SDL_Color){80, 150, 80, 255}; // green

    // load from map button
    compileButtons[1].rect = (SDL_Rect){
        WINDOW_WIDTH-UI_PANEL_WIDTH+button_padding, 
        button_start_y + (button_height + button_y_spacing),
        button_width, 
        button_height
    };
    strcpy(compileButtons[1].label, "LOAD FROM MAP");
    compileButtons[1].color = (SDL_Color){80, 80, 150, 255}; // blue

    // open map.txt button
    compileButtons[2].rect = (SDL_Rect){
        WINDOW_WIDTH-UI_PANEL_WIDTH+button_padding, 
        button_start_y + (button_height + button_y_spacing) * 2,
        button_width, 
        button_height
    };
    strcpy(compileButtons[2].label, "OPEN MAP.TXT");
    compileButtons[2].color = (SDL_Color){150, 80, 80, 255}; // red
}

void draw_compile_buttons() {
    for(int i=0; i<MAX_COMPILE_BUTTONS; i++) {
        // draw button background
        SDL_SetRenderDrawColor(renderer, 
            compileButtons[i].color.r,
            compileButtons[i].color.g,
            compileButtons[i].color.b,
            255
        );
        SDL_RenderFillRect(renderer, &compileButtons[i].rect);
        
        // draw button border
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        SDL_RenderDrawRect(renderer, &compileButtons[i].rect);
        
        // draw button label
        if (font != NULL) {
            SDL_Color textColor = {255, 255, 255, 255};
            SDL_Surface* textSurface = TTF_RenderText_Blended(font, compileButtons[i].label, textColor);
            if (textSurface != NULL) {
                SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
                if (textTexture != NULL) {
                    SDL_Rect textRect = {
                        compileButtons[i].rect.x + (compileButtons[i].rect.w - textSurface->w) / 2,
                        compileButtons[i].rect.y + (compileButtons[i].rect.h - textSurface->h) / 2,
                        textSurface->w,
                        textSurface->h
                    };
                    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
                    SDL_DestroyTexture(textTexture);
                }
                SDL_FreeSurface(textSurface);
            }
        }
    }
}

int check_file_exists(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file) {
        fclose(file);
        return 1; // file exists
    }
    return 0; // file doesn't exist
}

void save_map_to_file() {
    FILE *file = fopen(MAP_FILE_PATH, "w");
    if (!file) {
        char errorMsg[MAX_CONSOLE_LINE_LENGTH];
        snprintf(errorMsg, MAX_CONSOLE_LINE_LENGTH, "ERROR: Could not open file for writing: %s", MAP_FILE_PATH);
        append_to_console(errorMsg);
        
        // show error dialog
        strcpy(editor.confirmDialog.message, "Failed to write map file! Check permissions or disk space.");
        editor.confirmDialog.active = 1;
        editor.confirmDialog.onConfirm = NULL;
        editor.confirmDialog.userData = NULL;
        return;
    }
    
    // write map dimensions
    fprintf(file, "%d %d\n", editor.map_w, editor.map_h);
    
    // write map data
    for(int y = 0; y < editor.map_h; y++) {
        for(int x = 0; x < editor.map_w; x++) {
            fprintf(file, "%c", editor.cells[y][x]);
        }
        fprintf(file, "\n");
    }
    
    fclose(file);
    
    char successMsg[MAX_CONSOLE_LINE_LENGTH];
    snprintf(successMsg, MAX_CONSOLE_LINE_LENGTH, "Map saved to: %s", MAP_FILE_PATH);
    append_to_console(successMsg);
    
    // show success dialog
    strcpy(editor.confirmDialog.message, "Map successfully compiled and saved!");
    editor.confirmDialog.active = 1;
    editor.confirmDialog.onConfirm = NULL;
    editor.confirmDialog.userData = NULL;
}

void load_default_map() {
    // setup a simple default map
    editor.map_w = 10;
    editor.map_h = 10;
    
    for(int y = 0; y < GRID_SIZE; y++) {
        for(int x = 0; x < GRID_SIZE; x++) {
            if (y < editor.map_h && x < editor.map_w) {
                if (y == 0 || x == 0 || y == editor.map_h-1 || x == editor.map_w-1) {
                    editor.cells[y][x] = '1'; // wall
                } else if (y == 2 && x == 2) {
                    editor.cells[y][x] = '2'; // player
                    editor.player_exists = 1;
                    editor.player_pos.x = x;
                    editor.player_pos.y = y;
                } else if (y == 6 && x == 5) {
                    editor.cells[y][x] = '3'; // light
                } else {
                    editor.cells[y][x] = '0'; // empty
                }
            } else {
                editor.cells[y][x] = '0';
            }
        }
    }
    
    // center grid with new dimensions
    center_grid();
}

void load_map_from_file() {
    FILE *file = fopen(MAP_FILE_PATH, "r");
    if (!file) {
        printf("ERROR: Could not open file for reading: %s\n", MAP_FILE_PATH);
        show_map_not_found_dialog();
        return;
    }
    
    // reset player tracking
    editor.player_exists = 0;
    
    // read map dimensions
    int width, height;
    if (fscanf(file, "%d %d\n", &width, &height) != 2) {
        printf("ERROR: Invalid map format\n");
        fclose(file);
        load_default_map();
        return;
    }
    
    // validate map dimensions
    if (width < 1 || width > GRID_SIZE || height < 1 || height > GRID_SIZE) {
        printf("ERROR: Invalid map dimensions: %dx%d\n", width, height);
        fclose(file);
        load_default_map();
        return;
    }
    
    editor.map_w = width;
    editor.map_h = height;
    
    // clear map first
    for(int y = 0; y < GRID_SIZE; y++) {
        for(int x = 0; x < GRID_SIZE; x++) {
            editor.cells[y][x] = '0';
        }
    }
    
    // read map data
    char line[GRID_SIZE+2]; // +2 for newline and null terminator
    int y = 0;
    while(y < height && fgets(line, sizeof(line), file)) {
        int len = strlen(line);
        
        // remove newline if present
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
            len--;
        }
        
        // copy line to map cells
        for(int x = 0; x < len && x < width; x++) {
            editor.cells[y][x] = line[x];
            
            // track player position if found
            if (line[x] == '2') {
                editor.player_exists = 1;
                editor.player_pos.x = x;
                editor.player_pos.y = y;
            }
        }
        y++;
    }
    
    fclose(file);
    printf("Map loaded from: %s\n", MAP_FILE_PATH);
    
    // center grid with loaded dimensions
    center_grid();
}

void open_map_in_editor() {
    if (!check_file_exists(MAP_FILE_PATH)) {
        show_map_not_found_dialog();
        return;
    }
    
    // platform-specific commands to open a file
    #ifdef _WIN32
    system("start " MAP_FILE_PATH);
    #elif defined(__APPLE__)
    system("open " MAP_FILE_PATH);
    #else
    system("xdg-open " MAP_FILE_PATH);
    #endif
    
    printf("Opening map file in text editor: %s\n", MAP_FILE_PATH);
}

void show_map_not_found_dialog() {
    strcpy(editor.confirmDialog.message, "MAP NOT FOUND! DEFAULT MAP LOADED!");
    editor.confirmDialog.active = 1;
    editor.confirmDialog.onConfirm = load_default_map;
    editor.confirmDialog.userData = NULL;
}

void init_color_picker() {
    editor.colorPicker.active = 0;
    editor.colorPicker.currentColor = editor.lightColor;
    editor.colorPicker.originalColor = editor.lightColor;
    
    // position the color picker in the center of the screen
    editor.colorPicker.rect = (SDL_Rect){
        (WINDOW_WIDTH - COLOR_PICKER_WIDTH) / 2,
        (WINDOW_HEIGHT - COLOR_PICKER_HEIGHT) / 2,
        COLOR_PICKER_WIDTH,
        COLOR_PICKER_HEIGHT
    };
    
    // position the color preview box
    editor.colorPicker.previewRect = (SDL_Rect){
        editor.colorPicker.rect.x + COLOR_PICKER_WIDTH - 80,
        editor.colorPicker.rect.y + 30,
        60,
        60
    };
    
    // position the apply button
    editor.colorPicker.applyButton = (SDL_Rect){
        editor.colorPicker.rect.x + COLOR_PICKER_WIDTH - 160,
        editor.colorPicker.rect.y + COLOR_PICKER_HEIGHT - 50,
        70,
        30
    };
    
    // position the cancel button
    editor.colorPicker.cancelButton = (SDL_Rect){
        editor.colorPicker.rect.x + COLOR_PICKER_WIDTH - 80,
        editor.colorPicker.rect.y + COLOR_PICKER_HEIGHT - 50,
        70,
        30
    };
    
    // position rgb sliders and inputs (will implement these fully later)
    int slider_y_start = editor.colorPicker.rect.y + 30;
    int slider_height = 20;
    int slider_spacing = 35;
    int slider_width = COLOR_PICKER_WIDTH - 140;
    int input_width = 45;
    
    for (int i = 0; i < 3; i++) {
        // slider
        editor.colorPicker.sliders[i] = (SDL_Rect){
            editor.colorPicker.rect.x + 30,
            slider_y_start + i * slider_spacing,
            slider_width,
            slider_height
        };
        
        // text input
        editor.colorPicker.inputs[i].rect = (SDL_Rect){
            editor.colorPicker.rect.x + 40 + slider_width,
            slider_y_start + i * slider_spacing,
            input_width,
            slider_height
        };
        
        // default to white (255)
        sprintf(editor.colorPicker.inputs[i].text, "255");
        editor.colorPicker.inputs[i].active = 0;
        editor.colorPicker.inputs[i].cursorPosition = 3; // position at end
        editor.colorPicker.inputs[i].maxLength = 3;      // 0-255 (3 digits max)
        editor.colorPicker.inputs[i].selectionStart = -1;
        editor.colorPicker.inputs[i].selectionEnd = -1;
    }
}

void draw_color_picker(ColorPicker* picker) {
    if (!picker->active) return;
    
    // semi-transparent overlay
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 192);
    SDL_Rect overlay = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
    SDL_RenderFillRect(renderer, &overlay);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    
    // dialog background
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    SDL_RenderFillRect(renderer, &picker->rect);
    
    // dialog border
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderDrawRect(renderer, &picker->rect);
    
    // title
    if (font != NULL) {
        SDL_Color textColor = {255, 255, 255, 255};
        SDL_Surface* titleSurface = TTF_RenderText_Blended(font, "LIGHT COLOR", textColor);
        if (titleSurface != NULL) {
            SDL_Texture* titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
            if (titleTexture != NULL) {
                SDL_Rect titleRect = {
                    picker->rect.x + 20,
                    picker->rect.y + 10,
                    titleSurface->w,
                    titleSurface->h
                };
                SDL_RenderCopy(renderer, titleTexture, NULL, &titleRect);
                SDL_DestroyTexture(titleTexture);
            }
            SDL_FreeSurface(titleSurface);
        }
    }
    
    // color preview
    SDL_SetRenderDrawColor(renderer, 
        picker->currentColor.r, 
        picker->currentColor.g, 
        picker->currentColor.b, 
        255);
    SDL_RenderFillRect(renderer, &picker->previewRect);
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderDrawRect(renderer, &picker->previewRect);
    
    // apply button
    SDL_SetRenderDrawColor(renderer, 80, 150, 80, 255);
    SDL_RenderFillRect(renderer, &picker->applyButton);
    SDL_SetRenderDrawColor(renderer, 100, 180, 100, 255);
    SDL_RenderDrawRect(renderer, &picker->applyButton);
    
    // cancel button
    SDL_SetRenderDrawColor(renderer, 150, 80, 80, 255);
    SDL_RenderFillRect(renderer, &picker->cancelButton);
    SDL_SetRenderDrawColor(renderer, 180, 100, 100, 255);
    SDL_RenderDrawRect(renderer, &picker->cancelButton);
    
    // button text
    if (font != NULL) {
        SDL_Color textColor = {255, 255, 255, 255};
        
        // apply button text
        SDL_Surface* applySurface = TTF_RenderText_Blended(font, "APPLY", textColor);
        if (applySurface != NULL) {
            SDL_Texture* applyTexture = SDL_CreateTextureFromSurface(renderer, applySurface);
            if (applyTexture != NULL) {
                SDL_Rect textRect = {
                    picker->applyButton.x + (picker->applyButton.w - applySurface->w) / 2,
                    picker->applyButton.y + (picker->applyButton.h - applySurface->h) / 2,
                    applySurface->w,
                    applySurface->h
                };
                SDL_RenderCopy(renderer, applyTexture, NULL, &textRect);
                SDL_DestroyTexture(applyTexture);
            }
            SDL_FreeSurface(applySurface);
        }
        
        // cancel button text
        SDL_Surface* cancelSurface = TTF_RenderText_Blended(font, "CANCEL", textColor);
        if (cancelSurface != NULL) {
            SDL_Texture* cancelTexture = SDL_CreateTextureFromSurface(renderer, cancelSurface);
            if (cancelTexture != NULL) {
                SDL_Rect textRect = {
                    picker->cancelButton.x + (picker->cancelButton.w - cancelSurface->w) / 2,
                    picker->cancelButton.y + (picker->cancelButton.h - cancelSurface->h) / 2,
                    cancelSurface->w,
                    cancelSurface->h
                };
                SDL_RenderCopy(renderer, cancelTexture, NULL, &textRect);
                SDL_DestroyTexture(cancelTexture);
            }
            SDL_FreeSurface(cancelSurface);
        }
        
        // rgb labels
        const char* labels[3] = {"R:", "G:", "B:"};
        for (int i = 0; i < 3; i++) {
            SDL_Surface* labelSurface = TTF_RenderText_Blended(font, labels[i], textColor);
            if (labelSurface != NULL) {
                SDL_Texture* labelTexture = SDL_CreateTextureFromSurface(renderer, labelSurface);
                if (labelTexture != NULL) {
                    SDL_Rect labelRect = {
                        picker->sliders[i].x - 20,
                        picker->sliders[i].y + (picker->sliders[i].h - labelSurface->h) / 2,
                        labelSurface->w,
                        labelSurface->h
                    };
                    SDL_RenderCopy(renderer, labelTexture, NULL, &labelRect);
                    SDL_DestroyTexture(labelTexture);
                }
                SDL_FreeSurface(labelSurface);
            }
        }
    }
}

void handle_color_picker(ColorPicker* picker, SDL_Event* e) {
    if (!picker->active) return;

    int mx, my;
    SDL_GetMouseState(&mx, &my);

    if (e->type == SDL_MOUSEBUTTONDOWN) {
        if (SDL_PointInRect(&(SDL_Point){mx, my}, &picker->applyButton)) {
            editor.lightColor = picker->currentColor;
            tools[TOOL_LIGHT].color = picker->currentColor;
            picker->active = 0;
        } else if (SDL_PointInRect(&(SDL_Point){mx, my}, &picker->cancelButton)) {
            picker->currentColor = picker->originalColor;
            picker->active = 0;
        }
    }
}

int is_light_color(SDL_Color color) {
    float brightness = (0.299f * color.r + 0.587f * color.g + 0.114f * color.b) / 255.0f;
    return brightness > 0.6f; // Threshold for light vs dark
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() == -1) {
        printf("TTF could not initialize! TTF_Error: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    window = SDL_CreateWindow("MOON Editor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 14);
    if (!font) {
        printf("Failed to load font! TTF_Error: %s\n", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    // set font hinting
    TTF_SetFontHinting(font, TTF_HINTING_LIGHT);

    init_editor();

    SDL_Event e;
    int running = 1;
    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = 0;
            } else if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP || e.type == SDL_MOUSEMOTION) {
                handle_mouse(&e);
            } else if (e.type == SDL_MOUSEWHEEL) {
                handle_mouse_wheel(&e);
            } else if (e.type == SDL_TEXTINPUT || e.type == SDL_KEYDOWN) {
                for (int i = 0; i < MAX_INPUT_FIELDS; i++) {
                    handle_text_input(&editor.sizeInputs[i], &e);
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        draw_grid();
        draw_blocks();
        draw_ui();
        draw_console();

        SDL_RenderPresent(renderer);
    }

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}
