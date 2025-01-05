#include<stdio.h>
#include<stdlib.h>
#include<SDL.h>
#include<stdbool.h>

#include "./la.h"

#define STB_IMAGE_IMPLEMENTATION
#include "./stb_image.h"

#define FONT_COLS 18
#define FONT_ROWS 7
#define FONT_WIDTH 128
#define FONT_HEIGHT 64
#define FONT_CHAR_WIDTH (FONT_WIDTH / FONT_COLS)
#define FONT_CHAR_HEIGHT (FONT_HEIGHT / FONT_ROWS)
#define FONT_SCALE 5

#define ASCII_DISPLAY_LOW 32
#define ASCII_DISPLAY_HIGH 126

#define BUFFER_CAPACITY 1024
#define FONT_COLOR 0xFFFFFFFF

#define UNHEXRGBA(color) \
(color >> (8 * 0)) & 0xff, \
(color >> (8 * 1)) & 0xff, \
(color >> (8 * 2)) & 0xff, \
(color >> (8 * 3)) & 0xff

#define UNHEXRGB(color) \
(color >> (8 * 0)) & 0xff, \
(color >> (8 * 1)) & 0xff, \
(color >> (8 * 2)) & 0xff

void scc (int code) { 
    //sdl check code, checks output code of sdl functions, if error, writes that to stderr
    if (code<0){
        fprintf(stderr, "SDL Error: %s\n", SDL_GetError()); //fprintf is used to write to files, here we're writing to stderr file stream the sdl error to nicely handle error
        exit(1);
    }
}

void* scp(void* ptr){
    //sdl check pointer, checks the returned pointer from sdl functions, if null, the function failed and hence catch error
    if (ptr==NULL){
        fprintf(stderr, "SDL Error: %s\n", SDL_GetError()); //fprintf is used to write to files, here we're writing to stderr file stream the sdl error to nicely handle error
        exit(1);
    }
    return ptr; //return the original pointer
}

SDL_Surface* surface_from_file(const char *file_path){
    int width,height,n;
    unsigned char *pixels= stbi_load(file_path, &width, &height, &n,STBI_rgb_alpha);
    if (pixels==NULL){
        fprintf(stderr,"Could not load file %s: %s\n",file_path,stbi_failure_reason());
        exit(1);
    }

    #if SDL_BYTEORDER == SDL_BIG_ENDIAN
        const Uint32 rmask = 0xff000000;
        const Uint32 gmask = 0x00ff0000;
        const Uint32 bmask = 0x0000ff00;
        const Uint32 amask = 0x000000ff;
    #else //little endian like x86
        const Uint32 rmask = 0x000000ff;
        const Uint32 gmask = 0x0000ff00;
        const Uint32 bmask = 0x00ff0000;
        const Uint32 amask = 0xff000000;
    #endif

    const int depth =32;
    const int pitch = 4* width;

    return scp(SDL_CreateRGBSurfaceFrom((void*) pixels, width, height, depth, pitch, rmask, gmask, bmask, amask));
}

typedef struct{
    SDL_Texture* spritesheet;
    SDL_Rect glyph_table [ASCII_DISPLAY_HIGH - ASCII_DISPLAY_LOW + 1]; 
} Font;

Font font_load_from_file(SDL_Renderer* renderer, const char *file_path){
    Font font={0};
     SDL_Surface* font_surface= surface_from_file(file_path);
     font.spritesheet =scp(SDL_CreateTextureFromSurface(renderer,font_surface));
     SDL_FreeSurface(font_surface);

     for(size_t ascii = ASCII_DISPLAY_LOW; ascii <= ASCII_DISPLAY_HIGH; ++ascii){ //precalculates glyphh/size for all font characters
         const size_t index = ascii - ASCII_DISPLAY_LOW; 
         const size_t col = index % FONT_COLS;
         const size_t row = index / FONT_COLS;

         font.glyph_table[index] =(SDL_Rect) {
        .x=col*FONT_CHAR_WIDTH,
        .y=row*FONT_CHAR_HEIGHT,
        .w=FONT_CHAR_WIDTH,
        .h=FONT_CHAR_HEIGHT
        };
     }
    return font;
}

void render_char(SDL_Renderer* renderer, Font* font,char c,  Vec2f pos, float scale){

    const SDL_Rect dst={
        .x=(int) floorf(pos.x),
        .y=(int) floorf(pos.y),
        .w=(int) floorf(FONT_CHAR_WIDTH * scale),
        .h=(int) floorf(FONT_CHAR_HEIGHT * scale)
    };
    assert(c>= ASCII_DISPLAY_LOW);
    assert(c<=ASCII_DISPLAY_HIGH);

    const size_t index = c - ASCII_DISPLAY_LOW;  

    scc(SDL_RenderCopy(renderer,font->spritesheet,&font->glyph_table[index],&dst));

}

void render_text_sized(SDL_Renderer* renderer, Font* font,const char* text, size_t text_size,  Vec2f pos, Uint32 color, float scale){
    
    scc(SDL_SetTextureColorMod(font->spritesheet, UNHEXRGB(color)));
    scc(SDL_SetTextureAlphaMod(font->spritesheet, (color >> (8 * 3)) & 0xff)); //sets the color and alpha value for the given input string of text
    // to set for individual character simply move to render_char

    Vec2f pen= pos;

    for(size_t i=0; i<text_size; ++i){
        render_char(renderer,font, text[i],  pen, scale);
        pen.x += FONT_CHAR_WIDTH * scale;
    }
}

void render_text(SDL_Renderer* renderer, Font* font,const char* text,  Vec2f pos, Uint32 color, float scale){
    render_text_sized(renderer,font,text, strlen(text), pos,color, scale);
}

char buffer[BUFFER_CAPACITY];
size_t buffer_size=0;
size_t buffer_cursor=0;

void render_cursor(SDL_Renderer* renderer, Uint32 color){
    const SDL_Rect rect={
        .x=(int) floorf(buffer_cursor*FONT_CHAR_WIDTH*FONT_SCALE),
        .y=0 ,
        .w=FONT_CHAR_WIDTH * FONT_SCALE,
        .h=FONT_CHAR_HEIGHT * FONT_SCALE
    };

    scc(SDL_SetRenderDrawColor(renderer, UNHEXRGBA(color))); //try to understand how this function works
    scc(SDL_RenderFillRect(renderer, &rect));

}

int main (void){
    scc(SDL_Init(SDL_INIT_VIDEO));
    
    SDL_Window* window= scp(SDL_CreateWindow("Text Editor", 0,0, 800, 600, SDL_WINDOW_RESIZABLE));
    SDL_Renderer* renderer= scp(SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED));

    Font font = font_load_from_file(renderer, "./charmap-oldschool_white.png");
    //creating event loop
    

    bool quit=false;
    while (!quit){
        SDL_Event event={0}; //initialising the event struct
        while (SDL_PollEvent(&event)){
            switch (event.type){
                case SDL_QUIT: {
                    quit =true;
                } break;

                case SDL_KEYDOWN: {     //try to understand how eventhough sdl keydown catches everything text still goes to textinput case
                    printf("In event \n");
                    switch(event.key.keysym.sym){ 
                        case SDLK_BACKSPACE: {
                            printf("In backspace \n");
                            if (buffer_size > 0){
                                buffer_size -= 1;
                            }
                         } break;
                    }
                } break;

                case SDL_TEXTINPUT: {
                    printf(" Typing \n");
                    size_t text_size = strlen(event.text.text);
                    const size_t free_space = BUFFER_CAPACITY - buffer_size;
                    if (text_size>free_space){
                        text_size=buffer_size;
                    }
                    memcpy(buffer + buffer_size, event.text.text, text_size);
                    buffer_size += text_size;
                } break;
            }
        }
        scc(SDL_SetRenderDrawColor(renderer,0,0,0,0)); //sets the color to be used by the renderer for operations
        scc(SDL_RenderClear(renderer)); //clears the renderer with the set color

        render_text_sized(renderer, &font, buffer,buffer_size, vec2f(0.0, 0.0), FONT_COLOR, FONT_SCALE);
        buffer_cursor=buffer_size; //for now cursor is always at the end of the printed text, this will change as we introduce arrow keys
        render_cursor(renderer, FONT_COLOR);
        SDL_RenderPresent(renderer); //updates the screen
    }

    SDL_Quit();
    return 0;
}