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
#include <coreinit/memdefaultheap.h>
#include <coreinit/memory.h>

#include <cstring>
#include <stdbool.h>
#include <stdint.h>
#include <string>
#include <wchar.h>

#include <arrowTex_png.h>
#include <backgroundMusic_mp3.h>
#include <checkmarkTex_png.h>
#include <flagEurTex_png.h>
#include <flagEurUsaTex_png.h>
#include <flagJapTex_png.h>
#include <flagMultiTex_png.h>
#include <flagUnkTex_png.h>
#include <flagUsaTex_png.h>
#include <goodbyeTex_png.h>
#include <input.h>
#include <renderer.h>
#include <swkbd_wrapper.h>
#include <utils.h>
#include <menu/utils.h>

CVideo *renderer;
GuiFrame *window;
FreeTypeGX *font;
GuiImage *background;
uint32_t bgColor = SCREEN_COLOR_BLACK;
uint32_t width, height;
GuiSound *backgroundMusic;
bool rendererRunning = false;

int32_t spaceWidth;

GuiFrame *errorOverlay = NULL;
GuiImageData *arrowData;
GuiImageData *checkmarkData;
GuiImageData *flagData[6];

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

void textToFrame(int line, int column, const char *str)
{
	if(!rendererRunning)
		return;
	
	line += 2;
	line *= -FONT_SIZE;
	
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
			text->setPosition(column + FONT_SIZE, line);
			text->setMaxWidth(width - column, GuiText::DOTTED);
	}
	
	window->append(text);
}


void lineToFrame(int column, uint32_t color)
{
	if(!rendererRunning)
		return;
	
	GuiImage *line = new GuiImage(width - (FONT_SIZE << 1), 3, screenColorToGX2color(color), GuiImage::IMAGE_COLOR);
	line->setAlignment(ALIGN_TOP_CENTER);
	line->setPosition(0.0f, ((column + 2) * -FONT_SIZE) + ((FONT_SIZE >> 1) + 1));
	
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
	GX2Color co = screenColorToGX2color(SCREEN_COLOR_GRAY);
	int size = (lineEnd - lineStart) * FONT_SIZE;
	int y = ((lineStart + 2) * -FONT_SIZE) + (FONT_SIZE >> 1);
	
	GuiImage *box = new GuiImage(3, size, co, GuiImage::IMAGE_COLOR);
	box->setAlignment(ALIGN_TOP_LEFT);
	box->setPosition(FONT_SIZE, y);
	window->append(box);
	
	box = new GuiImage(3, size, co, GuiImage::IMAGE_COLOR);
	box->setAlignment(ALIGN_TOP_RIGHT);
	box->setPosition(-FONT_SIZE, y);
	window->append(box);
	
	// Background - we paint it on top of the gray lines as they look better that way
	co = screenColorToGX2color(SCREEN_COLOR_BLACK);
	co.a = 64;
	size -= 6;
	y -= 3;
	box = new GuiImage(width - (FONT_SIZE << 1) - 6, size, co, GuiImage::IMAGE_COLOR);
	box->setAlignment(ALIGN_TOP_CENTER);
	box->setPosition(0.0f, y);
	window->append(box);
}

void barToFrame(int line, int column, uint32_t width, float progress)
{
	if(!rendererRunning)
		return;
	
	int x = FONT_SIZE + (column * spaceWidth);
	int y = ((line + 1) * -FONT_SIZE) + 2;
	int height = FONT_SIZE;
	uint32_t tc = column + (width >> 1);
	width *= spaceWidth;
	
	GuiImage *bar = new GuiImage(width, height, screenColorToGX2color(SCREEN_COLOR_GRAY), GuiImage::IMAGE_COLOR);
	bar->setAlignment(ALIGN_TOP_LEFT);
	bar->setPosition(x, y);
	window->append(bar);
	
	x += 2;
	y -= 2;
	height -= 4;
	width -= 4;
	uint32_t barWidth = ((float)width) / 100.0f * progress; //TODO
	
	bar = new GuiImage(barWidth, height, screenColorToGX2color(SCREEN_COLOR_GREEN), GuiImage::IMAGE_COLOR);
	GX2Color co = screenColorToGX2color(SCREEN_COLOR_D_GREEN);
	for(int i = 1; i < 3; i++)
		bar->setImageColor(co, i);
	bar->setAlignment(ALIGN_TOP_LEFT);
	bar->setPosition(x, y);
	window->append(bar);
	
	width -= barWidth;
	x += barWidth;
	
	co = screenColorToGX2color(SCREEN_COLOR_BLACK);
	co.a = 64;
	bar = new GuiImage(width, height, co, GuiImage::IMAGE_COLOR);
	bar->setAlignment(ALIGN_TOP_LEFT);
	bar->setPosition(x, y);
	window->append(bar);
	
	char text[5];
	sprintf(text, "%d%%", (int)progress);
	tc -= strlen(text) >> 1;
	textToFrame(line, tc, text);
}

void arrowToFrame(int line, int column)
{
	line += 1;
	line *= -FONT_SIZE;
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
	line *= -FONT_SIZE;
	column *= spaceWidth;
	column += spaceWidth >> 1;
	
	GuiImage *checkmark = new GuiImage(checkmarkData);
	checkmark->setAlignment(ALIGN_TOP_LEFT);
	checkmark->setPosition(column + FONT_SIZE, line);
	window->append(checkmark);
}

GuiImageData *getFlagData(TITLE_REGION flag)
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
	line *= -FONT_SIZE;
	column *= spaceWidth;
	column += spaceWidth >> 1;
	
	GuiImage *image = new GuiImage(getFlagData(flag));
	image->setAlignment(ALIGN_TOP_LEFT);
	image->setPosition(column + FONT_SIZE, line);
	window->append(image);
}

void addErrorOverlay(const char *err)
{
	if(!rendererRunning)
		return;
	
	if(errorOverlay != NULL)
		return;
	
	errorOverlay = new GuiFrame(width, height);
	
	GX2Color bgColor = screenColorToGX2color(SCREEN_COLOR_BLACK);
	bgColor.a = 0xC0;
	GuiImage *img = new GuiImage(width, height, bgColor, GuiImage::IMAGE_COLOR);
	errorOverlay->append(img);
	
	const wchar_t *werr = FreeTypeGX::charToWideChar(err);
	GuiText *text = new GuiText(werr, FONT_SIZE, glm::vec4(1.0f), SSAA);
	text->setAlignment(ALIGN_CENTERED);
	
	uint16_t ow = font->getWidth(werr, FONT_SIZE) + (FONT_SIZE << 1);
	uint16_t oh = font->getHeight(werr, FONT_SIZE) + (FONT_SIZE << 1);
	
	img = new GuiImage(ow, oh, screenColorToGX2color(SCREEN_COLOR_RED), GuiImage::IMAGE_COLOR);
	img->setAlignment(ALIGN_CENTERED);
	errorOverlay->append(img);
	
	ow -= 2;
	oh -= 2;
	
	img = new GuiImage(ow, oh, screenColorToGX2color(SCREEN_COLOR_D_RED), GuiImage::IMAGE_COLOR);
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

void initRenderer()
{
	if(rendererRunning)
		return;
	
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
	size_t size;
	OSGetSharedData(OS_SHAREDDATATYPE_FONT_STANDARD, 0, (void **)&ttf, &size);
	font = new FreeTypeGX(ttf, size);
	spaceWidth = font->getCharWidth(L' ', FONT_SIZE);
	
	GuiText::setPresets(FONT_SIZE, glm::vec4({1.0f}), width - (FONT_SIZE << 1), ALIGN_TOP_LEFT, SSAA);
	GuiText::setPresetFont(font);
	
	background = new GuiImage(width, height, screenColorToGX2color(bgColor), GuiImage::IMAGE_COLOR);
	
	arrowData = new GuiImageData(arrowTex_png, arrowTex_png_size, GX2_TEX_CLAMP_MODE_WRAP , GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8 );
	checkmarkData = new GuiImageData(checkmarkTex_png, checkmarkTex_png_size, GX2_TEX_CLAMP_MODE_WRAP , GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8 );
	
	const uint8_t *tex;
	for(int i = 0; i < 5; i++)
	{
		switch(i)
		{
			case 0:
				tex = flagMultiTex_png;
				size = flagMultiTex_png_size;
				break;
			case 1:
				tex = flagEurTex_png;
				size = flagEurTex_png_size;
				break;
			case 2:
				tex = flagUsaTex_png;
				size = flagUsaTex_png_size;
				break;
			case 3:
				tex = flagJapTex_png;
				size = flagJapTex_png_size;
				break;
			case 4:
				tex = flagEurUsaTex_png;
				size = flagEurUsaTex_png_size;
				break;
			case 5:
				tex = flagUnkTex_png;
				size = flagUnkTex_png_size;
				break;
		}
		flagData[i] = new GuiImageData(tex, size, GX2_TEX_CLAMP_MODE_WRAP , GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8 );
	}
	
	rendererRunning = true;
	
	addToScreenLog("libgui initialized!");
	startNewFrame();
	textToFrame(0, 0, "Loading...");
	writeScreenLog();
	drawFrame();
	
	renderer->drcEnable(true);
	renderer->tvEnable(true);
	
	showFrame();
}

static inline void clearFrame()
{
	for(uint32_t i = 1; i < window->getSize(); i++)
		delete window->getGuiElementAt(i);
	
	window->removeAll();
}

void shutdownRenderer()
{
	if(!rendererRunning)
		return;
	
	colorStartNewFrame(SCREEN_COLOR_BLUE);
	
	GuiImageData *byeData = new GuiImageData(goodbyeTex_png, goodbyeTex_png_size, GX2_TEX_CLAMP_MODE_WRAP , GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8 );
	GuiImage *bye = new GuiImage(byeData);
	window->append(bye);
	
	drawFrame();
	clearFrame();
	removeErrorOverlay();
	
	backgroundMusic->Stop();
	delete backgroundMusic;
	delete renderer;
	delete window;
	delete background;
	delete font;
	delete bye;
	delete byeData;
	delete arrowData;
	delete checkmarkData;
	for(int i = 0; i < 6; i++)
		delete flagData[i];
	
	libgui_memoryRelease();
	bgColor = SCREEN_COLOR_BLACK;
	rendererRunning = false;
}

void colorStartNewFrame(uint32_t color)
{
	if(!rendererRunning)
		return;
	
	clearFrame();
	
//	if((color & 0xFFFFFF00) == 0xFFFFFF00 || (color & 0xFF) == 0x00)
//		color = SCREEN_COLOR_BLACK
	
	if(color != bgColor)
	{
		if(color == SCREEN_COLOR_BLUE)
		{
			background->setImageColor(screenColorToGX2color(SCREEN_COLOR_BG2), 0);
			background->setImageColor(screenColorToGX2color(SCREEN_COLOR_BG3), 1);
			background->setImageColor(screenColorToGX2color(SCREEN_COLOR_BG4), 2);
			background->setImageColor(screenColorToGX2color(SCREEN_COLOR_BG1), 3);
		}
		else
		{
			GX2Color gx2c = screenColorToGX2color(color);
			for(int i = 0; i < 4; i++)
				background->setImageColor(gx2c, i);
		}
		bgColor = color;
	}
	
	window->append(background);
}

void showFrame()
{
	if(!rendererRunning)
		return;
	
	GX2WaitForFlip();
	readInput();
}

// We need to draw the DRC before the TV, else the DRC is always one frame behind
void drawFrame()
{
	if(!rendererRunning)
		return;
	
	renderer->prepareDrcRendering();
	window->draw(renderer);
	if(errorOverlay != NULL)
		errorOverlay->draw(renderer);
	renderer->drcDrawDone();
	
	renderer->prepareTvRendering();
	window->draw(renderer);
	if(errorOverlay != NULL)
		errorOverlay->draw(renderer);
	renderer->tvDrawDone();
}

void drawKeyboard(bool tv)
{
	if(!rendererRunning)
		return;
	
	renderer->prepareDrcRendering();
	window->draw(renderer);
	if(tv)
		Swkbd_DrawTV();
	else
		Swkbd_DrawDRC();
	if(errorOverlay != NULL)
		errorOverlay->draw(renderer);
	renderer->drcDrawDone();
	
	renderer->prepareTvRendering();
	window->draw(renderer);
	if(tv)
		Swkbd_DrawTV();
	else
		Swkbd_DrawDRC();
	if(errorOverlay != NULL)
		errorOverlay->draw(renderer);
	renderer->tvDrawDone();
	
	showFrame();
}
