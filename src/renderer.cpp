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
#include <coreinit/memdefaultheap.h>
#include <coreinit/memory.h>

#include <cstring>
#include <stdbool.h>
#include <stdint.h>
#include <string>
#include <wchar.h>

#include <file.h>
#include <input.h>
#include <renderer.h>
#include <romfs.h>
#include <swkbd_wrapper.h>
#include <utils.h>
#include <menu/utils.h>

SDLSystem *sys;
GuiFrame *window;
GuiFont *font = NULL;
uint32_t width, height;
GuiSound *backgroundMusic = NULL;
bool rendererRunning = false;

int32_t spaceWidth;

void *backgroundMusicRaw = NULL;

GuiFrame *errorOverlay = NULL;
GuiTextureData *alphaData;
GuiTextureData *arrowData;
GuiTextureData *checkmarkData;
GuiTextureData *tabData;
GuiTextureData *flagData[6];

#define SSAA 8

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
	if(!rendererRunning)
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
			text->setMaxWidth(width - column);
	}
	
	window->append(text);
}


void lineToFrame(int column, uint32_t color)
{
	if(!rendererRunning)
		return;
	
	GuiImage *line = new GuiImage(screenColorToSDLcolor(color), width - (FONT_SIZE << 1), 3);
	line->setAlignment(ALIGN_TOP_CENTER);
	line->setPosition(0.0f, ((column + 1) * FONT_SIZE) + ((FONT_SIZE >> 1) - 1));
	
	window->append(line);
}

void boxToFrame(int lineStart, int lineEnd)
{
	if(!rendererRunning)
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
	size -= 4;
	y += 2;
	box = new GuiImage(alphaData);
	box->setScaleX(width - (FONT_SIZE << 1) - 6);
	box->setScaleY(size);
	box->setAlignment(ALIGN_TOP_CENTER);
	box->setPosition(0.0f, y);
	window->append(box);
}

void barToFrame(int line, int column, uint32_t width, float progress)
{
	if(!rendererRunning)
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
	uint32_t barWidth = ((float)width) / 100.0f * progress; //TODO
	
	bar = new GuiImage(screenColorToSDLcolor(SCREEN_COLOR_D_GREEN), barWidth, height);
	bar->setAlignment(ALIGN_TOP_LEFT);
	bar->setPosition(x, y);
	window->append(bar);
	
	width -= barWidth;
	x += barWidth;
	
	SDL_Color co = screenColorToSDLcolor(SCREEN_COLOR_BLACK);
	co.a = 64;
	bar = new GuiImage(alphaData);
	bar->setScaleX(width);
	bar->setScaleY(height);
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
	line += 1;
	line *= FONT_SIZE;
	column *= spaceWidth;
	column += spaceWidth;
	
	GuiImage *arrow = new GuiImage(arrowData);
	arrow->setAlignment(ALIGN_TOP_LEFT);
	arrow->setPosition(column + FONT_SIZE, line);
	window->append(arrow);
}

void checkmarkToFrame(int line, int column)
{
	line += 1;
	line *= FONT_SIZE;
	column *= spaceWidth;
	column += spaceWidth >> 1;
	
	GuiImage *checkmark = new GuiImage(checkmarkData);
	checkmark->setAlignment(ALIGN_TOP_LEFT);
	checkmark->setPosition(column + FONT_SIZE, line);
	window->append(checkmark);
}

GuiTextureData *getFlagData(TITLE_REGION flag)
{
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
	line += 1;
	line *= FONT_SIZE;
	column *= spaceWidth;
	column += spaceWidth >> 1;
	
	GuiImage *image = new GuiImage(getFlagData(flag));
	image->setAlignment(ALIGN_TOP_LEFT);
	image->setPosition(column + FONT_SIZE, line);
	window->append(image);
}

void tabToFrame(int line, int column, char *label, bool active)
{
	line *= FONT_SIZE;
	line += 9;
	column *= 240;
	column += 15;
	
	GuiImage *image = new GuiImage(tabData);
	image->setAlignment(ALIGN_TOP_LEFT);
	image->setPosition(column + FONT_SIZE, line);
	
	GuiText *text = new GuiText(label);
	
	line += 27;
	line -= FONT_SIZE >> 1;
	column += 120 - (((int)text->getWidth()) >> 1); //TODO
	
	text->setPosition(column + FONT_SIZE, line);
	text->setAlignment(ALIGN_TOP_LEFT);
	text->setMaxWidth(width - column);
	if(!active)
	{
		SDL_Color co;
		co.r = co.g = co.b = 255;
		co.a = 64;
		text->setColor(co);
	}
	window->append(image);
	window->append(text);
}

void addErrorOverlay(const char *err)
{
	if(!rendererRunning)
		return;
	
	if(errorOverlay != NULL)
		return;
	
	errorOverlay = new GuiFrame(width, height);
	
	SDL_Color bgColor = screenColorToSDLcolor(SCREEN_COLOR_BLACK);
	bgColor.a = 0xC0; // TODO
	GuiImage *img = new GuiImage(bgColor, width, height);
	img->setAlignment(ALIGN_CENTERED);
	errorOverlay->append(img);
	
	GuiText *text = new GuiText(err, FONT_SIZE, screenColorToSDLcolor(SCREEN_COLOR_WHITE)); // TODO: Add back SSAA
	text->setAlignment(ALIGN_CENTERED);

	// TODO
	uint16_t ow = text->getWidth();
	uint16_t oh = text->getHeight();
	
	img = new GuiImage(screenColorToSDLcolor(SCREEN_COLOR_RED), ow, oh);
	img->setAlignment(ALIGN_CENTERED);
	errorOverlay->append(img);
	
	ow -= 2;
	oh -= 2;
	
	img = new GuiImage(screenColorToSDLcolor(SCREEN_COLOR_D_RED), ow, oh);
	img->setAlignment(ALIGN_CENTERED);
	errorOverlay->append(img);
	
	errorOverlay->append(text);
	drawFrame();
}

void removeErrorOverlay()
{
	if(errorOverlay == NULL)
		return;
	
	for(int i = 0; i < 3; i++)
		delete errorOverlay->getGuiElementAt(i);
	
	delete errorOverlay;
	errorOverlay = NULL;
	drawFrame();
}

void resumeRenderer()
{
	if(rendererRunning)
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
	
	alphaData = new GuiTextureData(ROMFS_PATH "textures/alpha.png");
	arrowData = new GuiTextureData(ROMFS_PATH "textures/arrow.png"); //TODO: Error handling...
	checkmarkData = new GuiTextureData(ROMFS_PATH "textures/checkmark.png");
	tabData = new GuiTextureData(ROMFS_PATH "textures/tab.png");
	
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
	
	rendererRunning = true;
}

void initRenderer()
{
	if(rendererRunning)
		return;
	
	sys = new SDLSystem();
	
	backgroundMusic = new GuiSound(ROMFS_PATH "audio/bg.mp3");
	if(backgroundMusic != NULL)
	{
		backgroundMusic->SetLoop(true);
		backgroundMusic->SetVolume(15);
		backgroundMusic->Play();
	}
	
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

static inline void clearFrame()
{
//	while(window->getGuiElementAt(0) != nullptr)
//		delete window->getGuiElementAt(0);
	for(int32_t i = window->getSize() - 1; i > -1; i--)
		delete window->getGuiElementAt(i);
	
	window->removeAll();
}

void pauseRenderer()
{
	if(!rendererRunning)
		return;
	
	debugPrintf("Clearing frame");
	clearFrame();
	debugPrintf("Removing error overlay");
	removeErrorOverlay();
	
	debugPrintf("Deleting texture blobs");
	delete font;
	delete alphaData;
	delete arrowData;
	delete checkmarkData;
	delete tabData;
	
	for(int i = 0; i < 6; i++)
		delete flagData[i];
	
	debugPrintf("Deleting surface");
	delete window;
//	debugPrintf("Deleting renderer");
//	delete sys;
	
	rendererRunning = false;
}

void shutdownRenderer()
{
	if(!rendererRunning)
		return;
	
	debugPrintf("Starting new frame");
	colorStartNewFrame(SCREEN_COLOR_BLUE);
	
	debugPrintf("Opening PNG file");
	GuiTextureData *byeData = new GuiTextureData(ROMFS_PATH "textures/goodbye.png");
	GuiImage *bye = new GuiImage(byeData);
	bye->setAlignment(ALIGN_CENTERED);
	debugPrintf("Attaching texture to frame");
	window->append(bye);
	
	debugPrintf("Drawing frame");
	drawFrame();
	debugPrintf("Showing frame");
	showFrame();
	debugPrintf("Stopping renderer");
	pauseRenderer();
	
	debugPrintf("Deleting background music");
	if(backgroundMusic != NULL)
	{
		backgroundMusic->Stop();
		delete backgroundMusic;
		backgroundMusic = NULL;
	}
	delete byeData;
}

void colorStartNewFrame(uint32_t color)
{
	if(!rendererRunning)
		return;
	
	clearFrame();
	
	GuiImage *background = new GuiImage(screenColorToSDLcolor(color), width, height);
/*	if(color == SCREEN_COLOR_BLUE)
	{
		background->setImageColor(screenColorToSDLcolor(SCREEN_COLOR_BG2), 0);
		background->setImageColor(screenColorToSDLcolor(SCREEN_COLOR_BG3), 1);
		background->setImageColor(screenColorToSDLcolor(SCREEN_COLOR_BG4), 2);
		background->setImageColor(screenColorToSDLcolor(SCREEN_COLOR_BG1), 3);
	}*/
	window->append(background);
}

uint32_t lastTick = 0;
void showFrame()
{
	if(!rendererRunning)
		return;
	
	uint32_t now;
	do
	{
		SDL_Delay(1);
		now = SDL_GetTicks();
	}
	while(now - lastTick < 1000 / 60);
	
	lastTick = now;
	readInput();
}

// We need to draw the DRC before the TV, else the DRC is always one frame behind
void drawFrame()
{
	if(!rendererRunning)
		return;
	
	window->process();
	Renderer *renderer = sys->getRenderer();
	SDL_Renderer *sdlRenderer = renderer->getRenderer();
	SDL_RenderClear(sdlRenderer);
	window->draw(renderer);
	if(errorOverlay != NULL)
	{
		errorOverlay->process();
		errorOverlay->draw(renderer);
	}
	
	SDL_RenderPresent(sdlRenderer);
}

void drawKeyboard(bool tv)
{
	if(!rendererRunning)
		return;
	
	window->process();
	Renderer *renderer = sys->getRenderer();
	SDL_Renderer *sdlRenderer = renderer->getRenderer();
	SDL_RenderClear(sdlRenderer);
	window->draw(renderer);
	if(tv)
		Swkbd_DrawTV();
	else
		Swkbd_DrawDRC();
	if(errorOverlay != NULL)
		errorOverlay->draw(renderer);
	
	SDL_RenderPresent(sdlRenderer);
	showFrame();
}
