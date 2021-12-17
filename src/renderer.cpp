/***************************************************************************
 * This file is part of NUSspli.                                           *
 * Copyright (c) 2020-2021 V10lator <v10lator@myway.de>                    *
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

#include <SDL2/SDL.h>
#include <gui/GuiFont.h>
#include <gui/GuiFrame.h>
#include <gui/GuiImage.h>
#include <gui/GuiSound.h>
#include <gui/GuiText.h>
#include <gui/GuiTextureData.h>
#include <gui/system/SDLSystem.h>
#include <gui/video/Renderer.h>

#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/memory.h>
#include <whb/gfx.h>

#include <cstring>
#include <stdbool.h>
#include <stdint.h>
#include <string>
#include <wchar.h>

#include <crypto.h>
#include <file.h>
#include <input.h>
#include <renderer.h>
#include <romfs.h>
#include <swkbd_wrapper.h>
#include <utils.h>
#include <menu/utils.h>

#define SSAA            8
#define MAX_OVERLAYS    8

SDLSystem *sys;
GuiFrame *window = NULL;
Renderer *renderer;
SDL_Renderer *sdlRenderer;
GuiFont *font = NULL;
uint32_t width, height;
GuiSound *backgroundMusic = NULL;

int32_t spaceWidth;

GuiFrame *errorOverlay[MAX_OVERLAYS];
GuiTextureData *arrowData;
GuiTextureData *checkmarkData;
GuiTextureData *tabData;
GuiTextureData *flagData[6];
GuiTextureData *barData;
GuiTextureData *bgData;
GuiTextureData *byeData;

static inline SDL_Color screenColorToSDLcolor(uint32_t color)
{
	SDL_Color gx2color;
	
	gx2color.a = color & 0xFF;
	color >>= 8;
	gx2color.b = color & 0xFF;
	color >>= 8;
	gx2color.g = color & 0xFF;
	color >>= 8;
	gx2color.r = color & 0xFF;
	
	return gx2color;
}

void textToFrame(int line, int column, const char *str)
{
	if(window == NULL)
		return;
	
	line += 1;
	line *= FONT_SIZE;
	line -= 7;
	
	GuiText *text = new GuiText(str);
	
	switch(column)
	{
		case ALIGNED_CENTER:
			text->setAlignment(ALIGN_TOP_CENTER);
			text->setPosition(0.0f, line);
			break;
		case ALIGNED_RIGHT:
			text->setAlignment(ALIGN_TOP_RIGHT);
			text->setPosition(-FONT_SIZE, line);
			break;
		default:
			column *= spaceWidth;
			text->setAlignment(ALIGN_TOP_LEFT);
			text->setPosition(column + FONT_SIZE, line);
	}
	
	window->append(text);
}


void lineToFrame(int column, uint32_t color)
{
	if(window == NULL)
		return;
	
	GuiImage *line = new GuiImage(screenColorToSDLcolor(color), width - (FONT_SIZE << 1), 3);
	line->setAlignment(ALIGN_TOP_CENTER);
	line->setPosition(0.0f, ((column + 1) * FONT_SIZE) + ((FONT_SIZE >> 1) - 1));
	
	window->append(line);
}

void boxToFrame(int lineStart, int lineEnd)
{
	if(window == NULL)
		return;
	
	// Horizontal lines
	lineToFrame(lineStart, SCREEN_COLOR_GRAY);
	lineToFrame(lineEnd, SCREEN_COLOR_GRAY);
	
	// Vertical lines
	SDL_Color co = screenColorToSDLcolor(SCREEN_COLOR_GRAY);
	int size = (lineEnd - lineStart) * FONT_SIZE;
	int y = ((lineStart + 1) * FONT_SIZE) + (FONT_SIZE >> 1);
	
	GuiImage *box = new GuiImage(co, 3, size);
	box->setAlignment(ALIGN_TOP_LEFT);
	box->setPosition(FONT_SIZE, y);
	window->append(box);
	
	box = new GuiImage(co, 3, size);
	box->setAlignment(ALIGN_TOP_RIGHT);
	box->setPosition(-FONT_SIZE, y);
	window->append(box);
	
	// Background - we paint it on top of the gray lines as they look better that way
	co = screenColorToSDLcolor(SCREEN_COLOR_BLACK);
	co.a = 64;
	size -= 3;
	y += 2;
	box = new GuiImage(co, width - (FONT_SIZE << 1) - 6, size);
	box->setBlendMode(SDL_BLENDMODE_BLEND);
	box->setAlignment(ALIGN_TOP_CENTER);
	box->setPosition(0.0f, y);
	window->append(box);
}

void barToFrame(int line, int column, uint32_t width, double progress)
{
	if(window == NULL)
		return;
	
	int x = FONT_SIZE + (column * spaceWidth);
	int y = ((line + 1) * FONT_SIZE) - 2;
	int height = FONT_SIZE;
	uint32_t tc = column + (width >> 1);
	width *= spaceWidth;
	
	GuiImage *bar = new GuiImage(screenColorToSDLcolor(SCREEN_COLOR_GRAY), width, height);
	bar->setAlignment(ALIGN_TOP_LEFT);
	bar->setPosition(x, y);
	window->append(bar);
	
	x += 2;
	y += 2;
	height -= 4;
	width -= 4;
	uint32_t barWidth = ((double)width) / 100.0D * progress; //TODO
	
	bar = new GuiImage(barData);
	bar->setScaleQuality(SCALE_LINEAR);
	bar->setScaleX(((float)barWidth) / 2.0f);
	bar->setScaleY(height);
//	bar = new GuiImage(screenColorToSDLcolor(SCREEN_COLOR_GREEN), barWidth, height);
	bar->setAlignment(ALIGN_TOP_LEFT);
	bar->setPosition(x, y);
	window->append(bar);
	
	width -= barWidth;
	x += barWidth;
	
	SDL_Color co = screenColorToSDLcolor(SCREEN_COLOR_BLACK);
	co.a = 64;
	bar = new GuiImage(co, width, height);
	bar->setBlendMode(SDL_BLENDMODE_BLEND);
	bar->setAlignment(ALIGN_TOP_LEFT);
	bar->setPosition(x, y);
	window->append(bar);
	
	char text[5];
	sprintf(text, "%d%%%%", (int)progress);
	tc -= strlen(text) >> 1;
	textToFrame(line, tc, text);
}

void arrowToFrame(int line, int column)
{
	if(window == NULL)
		return;
	
	line += 1;
	line *= FONT_SIZE;
	column *= spaceWidth;
	column += spaceWidth;
	
	GuiImage *arrow = new GuiImage(arrowData);
	arrow->setBlendMode(SDL_BLENDMODE_BLEND);
	arrow->setAlignment(ALIGN_TOP_LEFT);
	arrow->setPosition(column + FONT_SIZE, line);
	window->append(arrow);
}

void checkmarkToFrame(int line, int column)
{
	if(window == NULL)
		return;
	
	line += 1;
	line *= FONT_SIZE;
	column *= spaceWidth;
	column += spaceWidth >> 1;
	
	GuiImage *checkmark = new GuiImage(checkmarkData);
	checkmark->setBlendMode(SDL_BLENDMODE_BLEND);
	checkmark->setAlignment(ALIGN_TOP_LEFT);
	checkmark->setPosition(column + FONT_SIZE, line);
	window->append(checkmark);
}

GuiTextureData *getFlagData(TITLE_REGION flag)
{
	if(window == NULL)
		return NULL;
	
	switch(flag)
	{
		case TITLE_REGION_ALL:
			return flagData[0];
		case TITLE_REGION_EUR:
			return flagData[1];
		case TITLE_REGION_USA:
			return flagData[2];
		case TITLE_REGION_JAP:
			return flagData[3];
		case TITLE_REGION_EUR | TITLE_REGION_USA:
			return flagData[4];
		default:
			return flagData[5];
	}
}

void flagToFrame(int line, int column, TITLE_REGION flag)
{
	if(window == NULL)
		return;
	
	line += 1;
	line *= FONT_SIZE;
	column *= spaceWidth;
	column += spaceWidth >> 1;
	
	GuiImage *image = new GuiImage(getFlagData(flag));
	image->setBlendMode(SDL_BLENDMODE_BLEND);
	image->setAlignment(ALIGN_TOP_LEFT);
	image->setPosition(column + FONT_SIZE, line);
	window->append(image);
}

void tabToFrame(int line, int column, char *label, bool active)
{
	if(window == NULL)
		return;
	
	line *= FONT_SIZE;
	line += 20;
	column *= 240;
	column += 13;
	
	GuiImage *image = new GuiImage(tabData);
	image->setBlendMode(SDL_BLENDMODE_BLEND);
	image->setAlignment(ALIGN_TOP_LEFT);
	image->setPosition(column + FONT_SIZE, line);
	
	GuiText *text = new GuiText(label);
	
	line += 20;
	line -= FONT_SIZE >> 1;
	column += 120 - (((int)text->getWidth()) >> 1); //TODO
	
	text->setPosition(column + FONT_SIZE, line);
	text->setAlignment(ALIGN_TOP_LEFT);
	if(!active)
	{
		SDL_Color co;
		co.r = co.g = co.b = 255;
		co.a = 159;
		text->setColor(co);
	}
	window->append(image);
	window->append(text);
}

int addErrorOverlay(const char *err)
{
	OSTick t = OSGetTick();
	addEntropy(&t, sizeof(OSTick));
	if(window == NULL)
		return -1;

	int i = 0;
	for( ; i < MAX_OVERLAYS + 1; i++)
		if(i < MAX_OVERLAYS && errorOverlay[i] == NULL)
			break;

	if(i == MAX_OVERLAYS)
		return -2;
	
	errorOverlay[i] = new GuiFrame(width, height);
	
	SDL_Color bgColor = screenColorToSDLcolor(SCREEN_COLOR_BLACK);
	bgColor.a = 0xC0; // TODO
	GuiImage *img = new GuiImage(bgColor, width, height);
	img->setBlendMode(SDL_BLENDMODE_BLEND);
	img->setAlignment(ALIGN_CENTERED);
	errorOverlay[i]->append(img);
	
	GuiText *text = new GuiText(err, FONT_SIZE, screenColorToSDLcolor(SCREEN_COLOR_WHITE)); // TODO: Add back SSAA
	text->setAlignment(ALIGN_CENTERED);

	uint16_t ow = text->getWidth() + FONT_SIZE;
	uint16_t oh = text->getHeight() + FONT_SIZE;
	
	img = new GuiImage(screenColorToSDLcolor(SCREEN_COLOR_RED), ow, oh);
	img->setAlignment(ALIGN_CENTERED);
	errorOverlay[i]->append(img);
	
	ow -= 4;
	oh -= 4;
	
	img = new GuiImage(screenColorToSDLcolor(SCREEN_COLOR_D_RED), ow, oh);
	img->setAlignment(ALIGN_CENTERED);
	errorOverlay[i]->append(img);
	
	errorOverlay[i]->append(text);
	drawFrame();
    return i;
}

void removeErrorOverlay(int id)
{
	OSTick t = OSGetTick();
	addEntropy(&t, sizeof(OSTick));
	if(id < 0 || id >= MAX_OVERLAYS || errorOverlay[id] == NULL)
		return;
	
	for(int32_t i = errorOverlay[id]->getSize() - 1; i > -1; i--)
		delete errorOverlay[id]->getGuiElementAt(i);
	
	delete errorOverlay[id];
	errorOverlay[id] = NULL;
	drawFrame();
}

void resumeRenderer()
{
	if(window != NULL)
		return;
	
	width = sys->getWidth();
	height = sys->getHeight();
	
	window = new GuiFrame(width, height);
	
	void *ttf;
	size_t size;
	OSGetSharedData(OS_SHAREDDATATYPE_FONT_STANDARD, 0, &ttf, &size);
	font = new GuiFont((uint8_t *)ttf, size, sys->getRenderer());
	
	GuiText::setPresets(FONT_SIZE, screenColorToSDLcolor(SCREEN_COLOR_WHITE)); // TODO
	GuiText::setPresetFont(font);
	
	GuiText *space = new GuiText(" ");
	spaceWidth = space->getWidth();
	delete space;
	
	OSTime t = OSGetSystemTime();
	arrowData = new GuiTextureData(ROMFS_PATH "textures/arrow.png"); //TODO: Error handling...
	checkmarkData = new GuiTextureData(ROMFS_PATH "textures/checkmark.png");
	tabData = new GuiTextureData(ROMFS_PATH "textures/tab.png");
	barData = new GuiTextureData(ROMFS_PATH "textures/bar.png");
	bgData = new GuiTextureData(ROMFS_PATH "textures/bg.png");
	
	const char *tex;
	for(int i = 0; i < 6; i++)
	{
		switch(i)
		{
			case 0:
				tex = ROMFS_PATH "textures/flags/multi.png";
				break;
			case 1:
				tex = ROMFS_PATH "textures/flags/eur.png";
				break;
			case 2:
				tex = ROMFS_PATH "textures/flags/usa.png";
				break;
			case 3:
				tex = ROMFS_PATH "textures/flags/jap.png";
				break;
			case 4:
				tex = ROMFS_PATH "textures/flags/eurUsa.png";
				break;
			case 5:
				tex = ROMFS_PATH "textures/flags/unk.png";
				break;
		}
		flagData[i] = new GuiTextureData(tex);
	}

	t = OSGetSystemTime() - t;
	addEntropy(&t, sizeof(OSTime));
}

void initRenderer()
{
	if(window != NULL)
		return;

	for(int i = 0; i < MAX_OVERLAYS; i++)
		errorOverlay[i] = NULL;
	
	sys = new SDLSystem();
	renderer = sys->getRenderer();
	sdlRenderer = renderer->getRenderer();
	
	OSTime t = OSGetSystemTime();
	byeData = new GuiTextureData(ROMFS_PATH "textures/goodbye.png");
	backgroundMusic = new GuiSound(ROMFS_PATH "audio/bg.mp3");
	if(backgroundMusic != NULL)
	{
		backgroundMusic->SetLoop(true);
		backgroundMusic->SetVolume(15);
		backgroundMusic->Play();
	}
	t = OSGetSystemTime() - t;
	addEntropy(&t, sizeof(OSTime));
	
	resumeRenderer();
	
	addToScreenLog("libgui-sdl initialized!");
	startNewFrame();
	textToFrame(0, 0, "Loading...");
	writeScreenLog();
	drawFrame();
	
//	renderer->drcEnable(true);
//	renderer->tvEnable(true);
	
	showFrame();
}

#define clearFrame()									\
	for(int32_t i = window->getSize() - 1; i > -1; i--)	\
		delete window->getGuiElementAt(i);				\
	window->removeAll();

void pauseRenderer()
{
	if(window == NULL)
		return;
	
	clearFrame();
	
	delete font;
	delete arrowData;
	delete checkmarkData;
	delete tabData;
	delete barData;
	delete bgData;
	
	for(int i = 0; i < 6; i++)
		delete flagData[i];
	
	delete window;
	window = NULL;
//	debugPrintf("Deleting renderer");
//	delete sys;
}

void shutdownRenderer()
{
	if(window == NULL)
		return;

	for(int i = 0; i < MAX_OVERLAYS; i++)
		removeErrorOverlay(i);
	
	colorStartNewFrame(SCREEN_COLOR_BLUE);
	
	GuiImage *bye = new GuiImage(byeData);
	bye->setBlendMode(SDL_BLENDMODE_BLEND);
	bye->setAlignment(ALIGN_CENTERED);
	window->append(bye);
	
	drawFrame();
	showFrame();
	clearFrame();
	delete(byeData);
	pauseRenderer();
	
	if(backgroundMusic != NULL)
	{
		debugPrintf("Stopping background music");
		backgroundMusic->Stop();
		delete backgroundMusic;
		backgroundMusic = NULL;
	}
}

void colorStartNewFrame(uint32_t color)
{
	if(window == NULL)
		return;
	
	clearFrame();
	
	GuiImage *background;
	if(color == SCREEN_COLOR_BLUE)
	{
		background = new GuiImage(bgData);
		background->setScaleQuality(SCALE_LINEAR);
		background->setScaleX(((float)width) / 2.0f);
		background->setScaleY(((float)height) / 2.0f);
	}
	else
		background = new GuiImage(screenColorToSDLcolor(color), width, height);

	window->append(background);
}

void showFrame()
{
	WHBGfxBeginRender();
	readInput();
}

// We need to draw the DRC before the TV, else the DRC is always one frame behind
void drawFrame()
{
	if(window == NULL)
		return;
	
	window->draw(renderer);
	for(int i = 0; i < MAX_OVERLAYS; i++)
		if(errorOverlay[i] != NULL)
			errorOverlay[i]->draw(renderer);
	
	SDL_RenderPresent(sdlRenderer);
}

void drawKeyboard(bool tv)
{
	if(window == NULL)
		return;

	window->draw(renderer);
	if(tv)
		Swkbd_DrawTV();
	else
		Swkbd_DrawDRC();
	for(int i = 0; i < MAX_OVERLAYS; i++)
		if(errorOverlay[i] != NULL)
			errorOverlay[i]->draw(renderer);
	
	SDL_RenderPresent(sdlRenderer);
	showFrame();
}
