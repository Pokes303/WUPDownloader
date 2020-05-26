/***************************************************************************
 * This file is part of NUSspli.                                           *
 * Copyright (c) 2020 V10lator <v10lator@myway.de>                         *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify    *
 * it under the terms of the GNU General Public License as published by    *
 * the Free Software Foundation; either version 2 of the License, or       *
 * (at your option) any later version.                                     *
 *                                                                         *
 * This program is distributed in the hope that it will be useful,         *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.             *
 ***************************************************************************/

#include <wut-fixups.h>

#include <glm/vec4.hpp>

#include <gx2/enum.h>

#include <gui/FreeTypeGX.h>
#include <gui/GuiFrame.h>
#include <gui/GuiImage.h>
#include <gui/GuiSound.h>
#include <gui/GuiText.h>
#include <gui/gx2_ext.h>
#include <gui/memory.h>
#include <gui/sounds/SoundHandler.hpp>
#include <gui/video/CVideo.h>

#include <coreinit/thread.h>

#include <cstring>
#include <stdbool.h>
#include <stdint.h>
#include <string>
#include <wchar.h>

#include <backgroundMusic_mp3.h>
#include <input.h>
#include <renderer.h>
#include <osdefs.h>
#include <swkbd_wrapper.h>
#include <utils.h>
#include <menu/utils.h>

CVideo *renderer;
GuiFrame *window;
FreeTypeGX *font;
GuiImage *background;
uint32_t bgColor = SCREEN_COLOR_BLUE;
uint32_t width, height;
GuiSound *backgroundMusic;

#define MAX_ELEMENTS MAX_LINES << 1
GuiElement *onScreen[MAX_ELEMENTS];
size_t onScreenIndex = 0;
int32_t spaceWidth;

#define SSAA 8

static inline GX2Color screenColorToGX2color(uint32_t color)
{
	GX2Color gx2color;
	
	gx2color.a = color & 0xFF;
	color >>= 8;
	gx2color.b = color & 0xFF;
	color >>= 8;
	gx2color.g = color & 0xFF;
	color >>= 8;
	gx2color.r = color & 0xFF;
	
	return gx2color;
}

void initRenderer()
{
	libgui_memoryInitialize();
	
	OSThread *audioThread = (OSThread *)SoundHandler::instance()->getThread();
	OSSetThreadName(audioThread, "NUSspli Audio");
	if(OSSetThreadPriority(audioThread, 0))
		addToScreenLog("Changed audio thread priority!");
	else
		addToScreenLog("WARNING: Error changing audio thread priority!");
	
	backgroundMusic = new GuiSound(backgroundMusic_mp3, backgroundMusic_mp3_size);
	backgroundMusic->SetLoop(true);
	backgroundMusic->SetVolume(15);
	backgroundMusic->Play();
	
	renderer = new CVideo(GX2_TV_SCAN_MODE_720P, GX2_DRC_RENDER_MODE_SINGLE);
	GX2SetSwapInterval(FRAMERATE_30FPS);
	
	width = renderer->getTvWidth();
	height = renderer->getTvHeight();
	
	window = new GuiFrame(width, height);
	
	FT_Bytes ttf;
	size_t ttfSize;
	OSGetSharedData(2, 0, &ttf, &ttfSize);
	font = new FreeTypeGX(ttf, ttfSize);
	spaceWidth = font->getCharWidth(L' ', FONT_SIZE);
	
	GuiText::setPresets(FONT_SIZE, glm::vec4({0.212f, 0.11f, 0.039f, 1.0f}), width - (FONT_SIZE << 1), ALIGN_TOP_LEFT, SSAA);
	GuiText::setPresetFont(font);
	
	background = new GuiImage(width, height, screenColorToGX2color(bgColor), GuiImage::IMAGE_COLOR);
	
	addToScreenLog("libgui initialized!");
	startNewFrame();
	textToFrame(0, 0, "Loading...");
	writeScreenLog();
	drawFrame();
	
	renderer->drcEnable(true);
	renderer->tvEnable(true);
	
	showFrame();
}

void shutdownRenderer()
{
	renderer->drcEnable(false);
	renderer->tvEnable(false);
	backgroundMusic->Stop();
	
	delete backgroundMusic;
	delete renderer;
	delete window;
	delete background;
	delete font;
	
	libgui_memoryRelease();
}

void colorStartNewFrame(uint32_t color)
{
//	if((color & 0xFFFFFF00) == 0xFFFFFF00 || (color & 0xFF) == 0x00)
//		color = SCREEN_COLOR_BLACK
	
	if(color != bgColor)
	{
		GX2Color gx2c = screenColorToGX2color(color);
		for(int i = 0; i < 4; i++)
			background->setImageColor(gx2c, i);
		
		bgColor = color;
	}
	
	if(color != SCREEN_COLOR_BLACK)
		window->append(background);
}

void showFrame()
{
	renderer->waitForVSync();
	readInput();
}

static inline void clearFrame()
{
	window->removeAll();
	while(onScreenIndex != 0)
		delete onScreen[--onScreenIndex];
}

// We need to draw the DRC before the TV, else the DRC is always one frame behind
void drawFrame()
{
	renderer->prepareDrcRendering();
	window->draw(renderer);
	renderer->drcDrawDone();
	
	renderer->prepareTvRendering();
	window->draw(renderer);
	renderer->tvDrawDone();
	
	clearFrame();
}

void drawKeyboard()
{
	colorStartNewFrame(bgColor);
	
	renderer->prepareDrcRendering();
	window->draw(renderer);
	Swkbd_DrawDRC();
	renderer->drcDrawDone();
	
	renderer->prepareTvRendering();
	window->draw(renderer);
	Swkbd_DrawTV();
	renderer->tvDrawDone();
	
	clearFrame();
	showFrame();
}

static inline void addToFrame(GuiElement *elm)
{
	if(onScreenIndex == MAX_ELEMENTS)
	{
		debugPrintf("MAX_ELEMENTS reached!");
		delete elm;
		return;
	}
	
	onScreen[onScreenIndex++] = elm;
	window->append(elm);
}

void textToFrame(int row, int column, const char *str)
{
	column += 2;
	column *= -FONT_SIZE;
	
	GuiText *text = new GuiText(str);
	
	if(row == ALIGNED_CENTER)
	{
		text->setAlignment(ALIGN_TOP_CENTER);
		text->setPosition(0.0f, column);
	}
	else if(row ==  ALIGNED_RIGHT)
	{
		text->setAlignment(ALIGN_TOP_RIGHT);
		text->setPosition(-FONT_SIZE, column);
	}
	else
	{
		int w = FONT_SIZE + (row * spaceWidth);
		text->setPosition(w, column);
		text->setMaxWidth(width - w - FONT_SIZE, GuiText::DOTTED);
	}
	
	addToFrame(text);
}


void lineToFrame(int column, uint32_t color)
{
	GuiImage *line = new GuiImage(width - (FONT_SIZE << 1), 3, screenColorToGX2color(color), GuiImage::IMAGE_COLOR);
	line->setAlignment(ALIGN_TOP_CENTER);
	line->setPosition(0.0f, ((column + 2) * -FONT_SIZE) + ((FONT_SIZE >> 1) + 1));
	
	addToFrame(line);
}

void boxToFrame(int lineStart, int lineEnd, uint32_t color)
{
	int size = (lineEnd - lineStart) * FONT_SIZE;
	
	int bw = width - (FONT_SIZE << 1);
	
	GuiImage *box = new GuiImage(bw, size, screenColorToGX2color(color), GuiImage::IMAGE_COLOR);
	box->setAlignment(ALIGN_TOP_CENTER);
	box->setPosition(0.0f, ((lineStart + 2) * -FONT_SIZE) + (FONT_SIZE >> 1));
	
	addToFrame(box);
	
	box = new GuiImage(bw - 6, size - 6, screenColorToGX2color(bgColor), GuiImage::IMAGE_COLOR);
	box->setAlignment(ALIGN_TOP_CENTER);
	box->setPosition(0.0f, ((lineStart + 2) * -FONT_SIZE) + ((FONT_SIZE >> 1) - 3));
	
	addToFrame(box);
}

void barToFrame(int line, int column, uint32_t width, float progress)
{
	int x = FONT_SIZE + (column * spaceWidth);
	int y = ((line + 1) * -FONT_SIZE) + 2;
	int height = FONT_SIZE;
	uint32_t tc = column + (width >> 1);
	width *= spaceWidth;
	
	GuiImage *bar = new GuiImage(width, height, screenColorToGX2color(SCREEN_COLOR_WHITE), GuiImage::IMAGE_COLOR);
	bar->setAlignment(ALIGN_TOP_LEFT);
	bar->setPosition(x, y);
	addToFrame(bar);
	
	x += 2;
	y -= 2;
	height -= 4;
	width -= 4;
	uint32_t barWidth = ((float)width) / 100.0f * progress; //TODO
	
	bar = new GuiImage(barWidth, height, screenColorToGX2color(SCREEN_COLOR_D_GREEN), GuiImage::IMAGE_COLOR);
	GX2Color lg = screenColorToGX2color(SCREEN_COLOR_GREEN);
	for(int i = 1; i < 3; i++)
		bar->setImageColor(lg, i);
	bar->setAlignment(ALIGN_TOP_LEFT);
	bar->setPosition(x, y);
	addToFrame(bar);
	
	width -= barWidth;
	x += barWidth;
	
	bar = new GuiImage(width, height, screenColorToGX2color(SCREEN_COLOR_GRAY), GuiImage::IMAGE_COLOR);
	bar->setAlignment(ALIGN_TOP_LEFT);
	bar->setPosition(x, y);
	addToFrame(bar);
	
	char text[5];
	sprintf(text, "%d%%", (int)progress);
	tc -= strlen(text) >> 1;
	textToFrame(tc, line, text);
}
