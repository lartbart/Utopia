#pragma once

#include "displaybackend.h"
#include "../core/canvas.h"
#include <linux/input.h>
#include <linux/fb.h>
#include <sys/select.h>
#include <fcntl.h>
#include <sys/mman.h> 
#include <sys/ioctl.h> 
#include <unistd.h>
#include "string.h"

class FramebufferBackend : public DisplayBackend {
	public:
		FramebufferBackend() = default;
		~FramebufferBackend() override = default;
		void init() override;
		void drawCanvas() override;
		bool getNextEvent() override;
		void setCanvasImage(CanvasImage canvas) override;
	private:
		int mouse = -1;
		int keyboard = -1;
	    struct fb_var_screeninfo vinfo;
    	struct fb_fix_screeninfo finfo;
		uint8_t* fbmem = nullptr;
		CanvasImage backend_canvas;
};
