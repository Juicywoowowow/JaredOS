#ifndef VBOX_DISPLAY_HPP
#define VBOX_DISPLAY_HPP

#ifdef __cplusplus

#include <SDL2/SDL.h>
#include <memory>

/**
 * VBoxDisplay - SDL2 Window wrapper with RAII using smart pointers
 *
 * This class manages the SDL2 window, renderer, and framebuffer texture.
 * All resources are automatically cleaned up via smart pointer destructors.
 */
class VBoxDisplay {
public:
  /**
   * Create display window
   * @param title Window title
   * @param width Window width in pixels
   * @param height Window height in pixels
   * @param scale Pixel scaling factor (default 2)
   */
  VBoxDisplay(const char *title, int width, int height, int scale = 2);
  ~VBoxDisplay();

  // Non-copyable, movable
  VBoxDisplay(const VBoxDisplay &) = delete;
  VBoxDisplay &operator=(const VBoxDisplay &) = delete;
  VBoxDisplay(VBoxDisplay &&) = default;
  VBoxDisplay &operator=(VBoxDisplay &&) = default;

  /**
   * Check if display initialized successfully
   */
  bool is_valid() const { return window_ != nullptr; }

  /**
   * Update display with VGA text mode buffer
   * @param vga_memory Pointer to VGA text buffer (0xB8000)
   * @param cols Number of columns
   * @param rows Number of rows
   */
  void update_text_mode(const uint8_t *vga_memory, int cols, int rows);

  /**
   * Update display with raw framebuffer
   * @param pixels RGBA pixel data
   * @param pitch Bytes per row
   */
  void update_framebuffer(const uint32_t *pixels, int pitch);

  /**
   * Poll SDL events and update keyboard state
   * @param key_pressed Output: ASCII code of pressed key (0 if none)
   * @param scancode Output: Raw scancode
   * @return false if window close requested
   */
  bool poll_events(uint8_t *key_pressed, uint8_t *scancode);

  /**
   * Present the current frame
   */
  void present();

  /**
   * Clear display to black
   */
  void clear();

  /* Accessors */
  int width() const { return width_; }
  int height() const { return height_; }
  int scale() const { return scale_; }

private:
  // Custom deleters for SDL resources
  struct SDLWindowDeleter {
    void operator()(SDL_Window *w) {
      if (w)
        SDL_DestroyWindow(w);
    }
  };
  struct SDLRendererDeleter {
    void operator()(SDL_Renderer *r) {
      if (r)
        SDL_DestroyRenderer(r);
    }
  };
  struct SDLTextureDeleter {
    void operator()(SDL_Texture *t) {
      if (t)
        SDL_DestroyTexture(t);
    }
  };

  std::unique_ptr<SDL_Window, SDLWindowDeleter> window_;
  std::unique_ptr<SDL_Renderer, SDLRendererDeleter> renderer_;
  std::unique_ptr<SDL_Texture, SDLTextureDeleter> framebuffer_;

  int width_;
  int height_;
  int scale_;

  // Text mode rendering buffer (RGBA)
  std::unique_ptr<uint32_t[]> text_buffer_;

  void render_text_char(int x, int y, uint8_t ch, uint8_t attr);
};

#endif /* __cplusplus */

/*============================================================================
 * C Interface (extern "C")
 *============================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque pointer for C code */
typedef struct VBoxDisplay VBoxDisplay;

/**
 * Create display window
 */
VBoxDisplay *display_create(const char *title, int width, int height,
                            int scale);

/**
 * Destroy display window
 */
void display_destroy(VBoxDisplay *display);

/**
 * Update display with VGA text mode buffer
 */
void display_update_text(VBoxDisplay *display, const uint8_t *vga_memory,
                         int cols, int rows);

/**
 * Poll events, returns 0 if quit requested
 */
int display_poll(VBoxDisplay *display, uint8_t *key_pressed, uint8_t *scancode);

/**
 * Present frame
 */
void display_present(VBoxDisplay *display);

/**
 * Clear display
 */
void display_clear(VBoxDisplay *display);

#ifdef __cplusplus
}
#endif

#endif /* VBOX_DISPLAY_HPP */
