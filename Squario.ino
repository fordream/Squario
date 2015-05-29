/*
	Written by Chris Martinez
	5/15/14
	*** Note: still super-unfinished.
*/

#include <SPI.h>
#include "Arduboy.h"
#include "resources.h"

Arduboy Arduboy;
void drawTile(int x, int _y, unsigned char image[]) {
	uint8_t offset = 0;
	uint8_t tail = 0;
	if (x+8 < 0) { return; } // offscreen
	if (x > 127) { return; } 	// offscreen
	if (x < 0 && x+8 >= 0) { offset =- x; x = 0; }
	if (x < 127 && x+8 > 127) { tail = x+8 - 127; }
	
	Arduboy.LCDCommandMode();
	SPI.transfer(0x20); 		// set display mode
	SPI.transfer(0x00);			// horizontal addressing mode
	SPI.transfer(0x21); 		// set col address
	unsigned char start = x;
	unsigned char end = x + 8 - tail;
	SPI.transfer(start & 0x7F);
	SPI.transfer(end & 0x7F);
	SPI.transfer(0x22); // set page address
	start = _y;
	end = _y;
	SPI.transfer(start & 0x07);
	SPI.transfer(end & 0x07);	
	Arduboy.LCDDataMode();
	for (uint8_t a = offset; a < 8 - tail; a++) { SPI.transfer(image[a]); }
}
struct Sprite {
	bool active;
	bool moving;
	//bool falling;
	
	int x;
	int y;
	int vx;
	int vy;
	
	uint8_t height;
	uint8_t tileTop;
	uint8_t tileBottom;
	uint8_t currentFrame;
	uint8_t maxFrame;
};
struct InteractiveObject {
	uint8_t x;
	uint8_t y;
};

const uint8_t spritesArraySize = 8;
Sprite sprites[spritesArraySize];

InteractiveObject iObject[255];
uint8_t iObjectIndex = 0;
uint8_t iObjectCount = 0;

int screenx = 0;
int fps = 1000/30;
long ltime;

bool up, down, left, right, aBut, bBut;

void setup() {
	Arduboy.start();
	ltime = millis();
	
	sprites[0].active = true;
	sprites[0].moving = true;
	sprites[0].height = 8;
	sprites[0].tileBottom = 1;
	
	sprites[0].currentFrame = 0;
	sprites[0].x = 20;
	sprites[0].y = 0;
	
	sprites[1].active = true;
	sprites[1].moving = false;
	sprites[1].height = 8;
	sprites[1].tileBottom = 4;
	sprites[1].currentFrame = 0;
	sprites[1].x = 21*8;
	sprites[1].y = 47;
}
void resetArrays() {
	for (uint8_t a = 0; a < 255; a++) {
		iObject[a].x = 0;
		iObject[a].y = 0;
	}
	iObjectIndex = 0;
	iObjectCount = 0;	
	for (uint8_t a = 0; a < spritesArraySize; a++) {
		sprites[a].active = false;
		sprites[a].moving = false;
	}
}
void die() {
	sprites[0].x = 20;
	sprites[0].y = 0;
	screenx = 0;
}
uint8_t collide(int x, int y) {
	uint8_t nX = x / 8;
	uint8_t nY = y / 8;
	switch (pgm_read_byte(&(level[nX][nY]))) {
		case 0: case 0xF0:
			return 0;
			break;
		case 2:
			for (uint8_t a = iObjectIndex; a < iObjectCount; a++) if (iObject[a].x == nX && iObject[a].y == nY) return 9;
			return 2; break;
		case 0x21:
			for (uint8_t a = iObjectIndex; a < iObjectCount; a++) if (iObject[a].x == nX && iObject[a].y == nY) return 9;
			return 0x21; break;
		case 3:	case 0x31:
			for (uint8_t a = iObjectIndex; a < iObjectCount; a++) if (iObject[a].x == nX && iObject[a].y == nY) return 0;
			return pgm_read_byte(&(level[nX][nY]));
		case 1:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
			return 1;
			break;
	}
}
void generateMushroom(uint8_t nX, uint8_t nY) {
	for (uint8_t a = 1; a < spritesArraySize; a++) {
		if (!sprites[a].active) {
			sprites[a].x = nX*8;
			sprites[a].y = (nY*8) - 8;
			sprites[a].vx = -1;
			sprites[a].height = 8;
			sprites[a].tileBottom = 5;
			sprites[a].active = true;
			sprites[a].moving = true;
			sprites[a].currentFrame = 0;
			return;
		}
	}
}
void eatMushroom(uint8_t i) {
	sprites[i].active = false;
	sprites[i].moving = false;
	
	if (sprites[0].height != 16) sprites[0].y = sprites[0].y - 8;
	sprites[0].height = 16;
	sprites[0].tileBottom = 3;
	sprites[0].tileTop = 2;
}
void headCollision() {
	uint8_t nX;
	uint8_t nY;
	uint8_t resultA;
	uint8_t resultB;
	if (sprites[0].x % 8 < 4) {
		resultA = collide(sprites[0].x, sprites[0].y-1);
		resultB = collide(sprites[0].x+8, sprites[0].y-1);
		if (resultA == 2 || resultA == 3 || resultA == 0x21 || resultA == 0x31) {
			nX = sprites[0].x/8;
			nY = (sprites[0].y-1)/8;
			if (pgm_read_byte(&(level[nX][nY])) == 2) { generateMushroom(nX,nY); breakObject(nX,nY); }
			else if (pgm_read_byte(&(level[nX][nY])) == 0x21) { breakObject(nX,nY); }
			else if (pgm_read_byte(&(level[nX][nY])) == 3 || pgm_read_byte(&(level[nX][nY])) == 0x31) {
				if (sprites[0].height == 16) breakObject(nX,nY);
			}
		}
		else if (resultB == 2 || resultB == 3 || resultB == 0x21 || resultB == 0x31) {
			nX = (sprites[0].x+8)/8;
			nY = (sprites[0].y-1)/8;
			if (pgm_read_byte(&(level[nX][nY])) == 2) { generateMushroom(nX,nY); breakObject(nX,nY); }
			else if (pgm_read_byte(&(level[nX][nY])) == 0x21) { breakObject(nX,nY); }
			else if (pgm_read_byte(&(level[nX][nY])) == 3 || pgm_read_byte(&(level[nX][nY])) == 0x31) {
				if (sprites[0].height == 16) breakObject(nX,nY);
			}
		}
	}
	else {
		resultA = collide(sprites[0].x+8, sprites[0].y-1);
		resultB = collide(sprites[0].x, sprites[0].y-1);
		if (resultA == 2 || resultA == 3 || resultA == 0x21 || resultA == 0x31) {
			nX = (sprites[0].x+8)/8;
			nY = (sprites[0].y-1)/8;
			if (pgm_read_byte(&(level[nX][nY])) == 2) { generateMushroom(nX,nY); breakObject(nX,nY); }
			else if (pgm_read_byte(&(level[nX][nY])) == 0x21) { breakObject(nX,nY); }
			else if (pgm_read_byte(&(level[nX][nY])) == 3 || pgm_read_byte(&(level[nX][nY])) == 0x31) {
				if (sprites[0].height == 16) breakObject(nX,nY);
			}
		} // qblock
		else if (resultB == 2 || resultB == 3 || resultB == 0x21 || resultB == 0x31) {
			nX = sprites[0].x/8;
			nY = (sprites[0].y-1)/8;
			if (pgm_read_byte(&(level[nX][nY])) == 2) { generateMushroom(nX,nY); breakObject(nX,nY); }
			else if (pgm_read_byte(&(level[nX][nY])) == 0x21) { breakObject(nX,nY); }
			else if (pgm_read_byte(&(level[nX][nY])) == 3 || pgm_read_byte(&(level[nX][nY])) == 0x31) {
				if (sprites[0].height == 16) breakObject(nX,nY);
			}
		} //qblock
	}
}
void breakObject(uint8_t nX, uint8_t nY) {
	iObject[iObjectCount].x = nX;
	iObject[iObjectCount].y = nY;
	iObjectCount++;
}
bool isIn(int x, int y, uint8_t i) {
	if (x >= sprites[i].x &&
		x <= sprites[i].x + 8 &&
		y >= sprites[i].y &&
		y <= sprites[i].y + 8)	return true;
	return false;
}
uint8_t spriteCollide(uint8_t i) {
	for (uint8_t a = 1; a < spritesArraySize; a++) {
		if (sprites[a].active && a != i) {
			if (isIn(sprites[i].x,  sprites[i].y,  a) ||
				isIn(sprites[i].x+8,sprites[i].y,  a) ||
				isIn(sprites[i].x,  sprites[i].y+sprites[i].height,a) ||
				isIn(sprites[i].x+8,sprites[i].y+sprites[i].height,a)) {
				if (i == 0 || a == 0) {
					if (sprites[i+a].tileBottom == 5) eatMushroom(i+a);
					else return i+a;
				}
				else { sprites[i].vx = sprites[i].vx * -1; }
			}
		}
	}
	return 0;
}

void squarioJump() {
	if (sprites[0].height == 8) {
		sprites[0].tileBottom = 0x11;
		sprites[0].currentFrame = 0;
		sprites[0].maxFrame = 0;
	}
	if (sprites[0].height == 16) {
	
	}
}
void squarioLand() {
	if (sprites[0].height == 8) {
		sprites[0].tileBottom = 0x01;
		sprites[0].currentFrame = 0;
		sprites[0].maxFrame = 2;
	}
	if (sprites[0].height == 16) {
	
	}
}

void moveSprite(uint8_t i) {
	if (sprites[i].vy == 0) {
		// check underneath
		if (!collide(sprites[i].x, sprites[i].y+sprites[i].height) && !collide(sprites[i].x+7, sprites[i].y+sprites[i].height)){
			sprites[i].vy++;
		}
	}
	if (sprites[i].vy > 0) { // falling
		for (int a = 0; a < sprites[i].vy; a++) {
			if (!collide(sprites[i].x, sprites[i].y+sprites[i].height) && !collide(sprites[i].x+7, sprites[i].y+sprites[i].height)) { sprites[i].y++; }
			else { sprites[i].vy = 0; }
			if (spriteCollide(i)) {
				if (sprites[spriteCollide(i)].tileBottom == 5) { }
				else { 
					sprites[spriteCollide(i)].active = false;
					sprites[spriteCollide(i)].moving = false;
				}
			}
		}
		if (sprites[i].vy > 0) sprites[i].vy = sprites[i].vy * 2;
	}
	if (sprites[i].vx > 0) { // Right
		for (int a = 0; a < sprites[i].vx; a++) {
			if (!collide(sprites[i].x + 8 + 1,sprites[i].y) && !collide(sprites[i].x + 8 + 1,sprites[i].y+sprites[i].height-1)) { sprites[i].x++; }
			else {
				if (i) sprites[i].vx = -1;
				else sprites[i].vx = 0;
			}
			if (spriteCollide(i)) {
				if (sprites[spriteCollide(i)].tileBottom == 5) { }
				else { die(); }
			}
		}
	}
	if (sprites[i].vx < 0) { // Left
		for (int a = 0; a > sprites[i].vx; a--) {
			if (!collide(sprites[i].x - 1,sprites[i].y) && !collide(sprites[i].x - 1,sprites[i].y+sprites[i].height-1)) {
				if (i || sprites[0].x-1 > screenx)	sprites[i].x--;
				if (i) { 
					if (sprites[i].x+8 < screenx) {
						sprites[i].active = false;
						sprites[i].moving = false;
					}
				}
			}
			else { 
				if (i) sprites[i].vx = 0;
				else sprites[i].vx = 0;
			}
			if (spriteCollide(i)) {
				if (sprites[spriteCollide(i)].tileBottom == 5) { }
				else { die(); }
			}
		}
	}
	if (sprites[i].vy < 0) { // up
		sprites[i].vy = sprites[i].vy * .55; // gravity
		for (int a = 0; a > sprites[i].vy; a--) {
			if (collide(sprites[i].x, sprites[i].y - 1) || collide(sprites[i].x+7, sprites[i].y -1)) {
				sprites[i].vy = 0;
				if (i == 0) headCollision();
			}
			else {
				if (sprites[i].y - 1 >= 0) sprites[i].y--;
				if (spriteCollide(i)) {
					if (sprites[spriteCollide(i)].tileBottom == 5) { }
					else { die(); }
				}
			}
		}
	}
	if (sprites[i].x + 8 < screenx) {
		sprites[i].active = false;
		sprites[i].moving = false;
	}
}
void drawMap() {
	unsigned char sBuffer[] = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
	uint8_t lastTile = 0;
	for (uint8_t x = screenx/8; x < (screenx/8) + 17; x++) {
		for (uint8_t y = 0; y < 8; y++) {
			uint8_t newTile = pgm_read_byte(&(level[x][y]));
			if (newTile == 2 || newTile == 0x21) for (uint8_t a = iObjectIndex; a < iObjectCount; a++) if (iObject[a].x == x && iObject[a].y == y) newTile = 9;
			if (newTile == 3 || newTile == 0x31) for (uint8_t a = iObjectIndex; a < iObjectCount; a++) if (iObject[a].x == x && iObject[a].y == y) newTile = 0;
			if (newTile != lastTile) {
				switch (newTile) {
					case 0:
					case 0xF0:	lastTile = 0; for (uint8_t a = 0; a < 8; a++) sBuffer[a] = pgm_read_byte(tBlank + a); break;
					case 1:		lastTile = 1; for (uint8_t a = 0; a < 8; a++) sBuffer[a] = pgm_read_byte(tGround + a); break;
					case 2:
					case 0x21:	lastTile = 2; for (uint8_t a = 0; a < 8; a++) sBuffer[a] = pgm_read_byte(tQBlock + a); break;
					case 3:		
					case 0x31:	lastTile = 3; for (uint8_t a = 0; a < 8; a++) sBuffer[a] = pgm_read_byte(tBrick + a); break;
					case 4:		lastTile = 4; for (uint8_t a = 0; a < 8; a++) sBuffer[a] = pgm_read_byte(tPipeLeft + a); break;
					case 5:		lastTile = 5; for (uint8_t a = 0; a < 8; a++) sBuffer[a] = pgm_read_byte(tPipeRight + a); break;
					case 6:		lastTile = 6; for (uint8_t a = 0; a < 8; a++) sBuffer[a] = pgm_read_byte(tPipeCapLeft + a); break;
					case 7:		lastTile = 7; for (uint8_t a = 0; a < 8; a++) sBuffer[a] = pgm_read_byte(tPipeCapRight + a); break;
					case 8:		lastTile = 8; for (uint8_t a = 0; a < 8; a++) sBuffer[a] = pgm_read_byte(tDSquare + a); break;
					case 9:		lastTile = 9; for (uint8_t a = 0; a < 8; a++) sBuffer[a] = pgm_read_byte(tBQBlock + a); break;
					case 10:	lastTile = 10; for (uint8_t a = 0; a < 8; a++) sBuffer[a] = pgm_read_byte(tFlagPole + a); break;
					case 11:	lastTile = 11; for (uint8_t a = 0; a < 8; a++) sBuffer[a] = pgm_read_byte(tFlagPoleTop + a); break;
				}
			}
			for (uint8_t b = 0; b < spritesArraySize; b++) {
				if (sprites[b].active) {
					uint8_t tile;
					if (sprites[b].height == 16) {
						tile = sprites[b].tileTop;
					}
					else tile = sprites[b].tileBottom;
					uint8_t nX = sprites[b].x / 8;
					uint8_t nY = sprites[b].y / 8;
					uint8_t xOffset = sprites[b].x % 8;
					uint8_t yOffset = sprites[b].y % 8;
					if (nY == y) {
						if (nX == x) {
							lastTile = 255; 
							for (uint8_t c = xOffset; c < 8; c++){
								switch (tile) {
									case 1: sBuffer[c] = sBuffer[c] | (pgm_read_byte(tSmallSquario + (c-xOffset)) << yOffset); break;
									case 2: sBuffer[c] = sBuffer[c] | (pgm_read_byte(tBigSquarioTop + (c-xOffset)) << yOffset); break;
									case 3: sBuffer[c] = sBuffer[c] | (pgm_read_byte(tBigSquarioBottom + (c-xOffset)) << yOffset); break;
									case 4: sBuffer[c] = sBuffer[c] | (pgm_read_byte(tSpademba + (c-xOffset)) << yOffset); break;
									case 5: sBuffer[c] = sBuffer[c] | (pgm_read_byte(tMushroom + (c-xOffset)) << yOffset); break;
								}
							}
						}
						nX = (sprites[b].x+8) / 8;
						if (nX == x) {
							lastTile = 255;
							uint8_t tail = 8 - (sprites[b].x % 8);
							for (uint8_t c = tail; c < 8; c++) {
								switch (tile) {
									case 1: sBuffer[c-tail] = sBuffer[c-tail] | (pgm_read_byte(tSmallSquario + c) << yOffset); break;
									case 2: sBuffer[c-tail] = sBuffer[c-tail] | (pgm_read_byte(tBigSquarioTop + c) << yOffset); break;
									case 3: sBuffer[c-tail] = sBuffer[c-tail] | (pgm_read_byte(tBigSquarioBottom + c) << yOffset); break;
									case 4: sBuffer[c-tail] = sBuffer[c-tail] | (pgm_read_byte(tSpademba + c) << yOffset); break;
									case 5: sBuffer[c-tail] = sBuffer[c-tail] | (pgm_read_byte(tMushroom + c) << yOffset); break;
								}
							}						
						}
					}
					nY = (sprites[b].y+8) / 8;
					if (nY == y) {
						yOffset = 8 - yOffset;
						if (nX == x) {
							lastTile = 255;
							for (uint8_t c = xOffset; c < 8; c++){
								switch (tile) {
									case 1: sBuffer[c] = sBuffer[c] | (pgm_read_byte(tSmallSquario + (c-xOffset)) >> (yOffset)); break;
									case 2: sBuffer[c] = sBuffer[c] | (pgm_read_byte(tBigSquarioTop + (c-xOffset)) >> (yOffset)); break;
									case 3: sBuffer[c] = sBuffer[c] | (pgm_read_byte(tBigSquarioBottom + (c-xOffset)) >> (yOffset)); break;
									case 4: sBuffer[c] = sBuffer[c] | (pgm_read_byte(tSpademba + (c-xOffset)) >> (yOffset)); break;
									case 5: sBuffer[c] = sBuffer[c] | (pgm_read_byte(tMushroom + (c-xOffset)) >> (yOffset)); break;
								}
							}											
						}
						nX = (sprites[b].x+8) / 8;
						if (nX == x) {
							uint8_t tail = 8 - (sprites[b].x % 8);
							lastTile = 255;
							for (uint8_t c = tail; c < 8; c++) {
								switch (tile) {
									case 1: sBuffer[c-tail] = sBuffer[c-tail] | (pgm_read_byte(tSmallSquario + c) >> yOffset); break;
									case 2: sBuffer[c-tail] = sBuffer[c-tail] | (pgm_read_byte(tBigSquarioTop + c) >> yOffset); break;
									case 3: sBuffer[c-tail] = sBuffer[c-tail] | (pgm_read_byte(tBigSquarioBottom + c) >> yOffset); break;
									case 4: sBuffer[c-tail] = sBuffer[c-tail] | (pgm_read_byte(tSpademba + c) >> yOffset); break;
									case 5: sBuffer[c-tail] = sBuffer[c-tail] | (pgm_read_byte(tMushroom + c) >> yOffset); break;
								}
							}						
						}
					}
					if (sprites[b].height == 16) {
						tile = sprites[b].tileBottom;
						nX = sprites[b].x / 8;
						nY = (sprites[b].y+8) / 8;
						xOffset = sprites[b].x % 8;
						yOffset = sprites[b].y % 8;
						if (nY == y) {
							if (nX == x) {
								lastTile = 255; 
								for (uint8_t c = xOffset; c < 8; c++){
									switch (tile) {
										case 1: sBuffer[c] = sBuffer[c] | (pgm_read_byte(tSmallSquario + (c-xOffset)) << yOffset); break;
										case 2: sBuffer[c] = sBuffer[c] | (pgm_read_byte(tBigSquarioTop + (c-xOffset)) << yOffset); break;
										case 3: sBuffer[c] = sBuffer[c] | (pgm_read_byte(tBigSquarioBottom + (c-xOffset)) << yOffset); break;
										case 4: sBuffer[c] = sBuffer[c] | (pgm_read_byte(tSpademba + (c-xOffset)) << yOffset); break;
										case 5: sBuffer[c] = sBuffer[c] | (pgm_read_byte(tMushroom + (c-xOffset)) << yOffset); break;
									}
								}
							}
							nX = (sprites[b].x+8) / 8;
							if (nX == x) {
								lastTile = 255;
								uint8_t tail = 8 - (sprites[b].x % 8);
								for (uint8_t c = tail; c < 8; c++) {
									switch (tile) {
										case 1: sBuffer[c-tail] = sBuffer[c-tail] | (pgm_read_byte(tSmallSquario + c) << yOffset); break;
										case 2: sBuffer[c-tail] = sBuffer[c-tail] | (pgm_read_byte(tBigSquarioTop + c) << yOffset); break;
										case 3: sBuffer[c-tail] = sBuffer[c-tail] | (pgm_read_byte(tBigSquarioBottom + c) << yOffset); break;
										case 4: sBuffer[c-tail] = sBuffer[c-tail] | (pgm_read_byte(tSpademba + c) << yOffset); break;
										case 5: sBuffer[c-tail] = sBuffer[c-tail] | (pgm_read_byte(tMushroom + c) << yOffset); break;
									}
								}					
							}
						}
						nY = (sprites[b].y+16) / 8;
						if (nY == y) {
							yOffset = 8 - yOffset;
							if (nX == x) {
								lastTile = 255;
								for (uint8_t c = xOffset; c < 8; c++){
									switch (tile) {
										case 1: sBuffer[c] = sBuffer[c] | (pgm_read_byte(tSmallSquario + (c-xOffset)) >> (yOffset)); break;
										case 2: sBuffer[c] = sBuffer[c] | (pgm_read_byte(tBigSquarioTop + (c-xOffset)) >> (yOffset)); break;
										case 3: sBuffer[c] = sBuffer[c] | (pgm_read_byte(tBigSquarioBottom + (c-xOffset)) >> (yOffset)); break;
										case 4: sBuffer[c] = sBuffer[c] | (pgm_read_byte(tSpademba + (c-xOffset)) >> (yOffset)); break;
										case 5: sBuffer[c] = sBuffer[c] | (pgm_read_byte(tMushroom + (c-xOffset)) >> (yOffset)); break;
									}
								}
							}
							nX = (sprites[b].x+8) / 8;
							if (nX == x) {
								uint8_t tail = 8 - (sprites[b].x % 8);
								lastTile = 255;
								for (uint8_t c = tail; c < 8; c++) {
									switch (tile) {
										case 1: sBuffer[c-tail] = sBuffer[c-tail] | (pgm_read_byte(tSmallSquario + c) >> yOffset); break;
										case 2: sBuffer[c-tail] = sBuffer[c-tail] | (pgm_read_byte(tBigSquarioTop + c) >> yOffset); break;
										case 3: sBuffer[c-tail] = sBuffer[c-tail] | (pgm_read_byte(tBigSquarioBottom + c) >> yOffset); break;
										case 4: sBuffer[c-tail] = sBuffer[c-tail] | (pgm_read_byte(tSpademba + c) >> yOffset); break;
										case 5: sBuffer[c-tail] = sBuffer[c-tail] | (pgm_read_byte(tMushroom + c) >> yOffset); break;
									}
								}
							}
						}	
					}
				}
			}
			drawTile(x*8 - screenx, y, sBuffer);
		}
	}
}
void loop() {
	long ctime = millis();
	if (ctime > ltime + fps) {
		if (left == false && right == false) {
			if (sprites[0].vx > 0) sprites[0].vx--;
			if (sprites[0].vx < 0) sprites[0].vx++;
		}
		if (left == true) {
			if (sprites[0].vx > -8) sprites[0].vx--;
			left = false; right = false;
		}
		if (right == true) {
			if (sprites[0].vx < 8) sprites[0].vx++;
			left = false; right = false;
		}
		if (aBut == true) {
			if (collide(sprites[0].x, sprites[0].y+sprites[0].height) || collide(sprites[0].x+8, sprites[0].y+sprites[0].height)) {
				sprites[0].vy = -32;
			}
			aBut = false;
		}
		// Do sprite things
		for (int a = 0; a < spritesArraySize; a++) {
			if (sprites[a].moving) moveSprite(a);
		}
		// Scroll the screen
		if (sprites[0].x - 56 > screenx) screenx = sprites[0].x - 56;
		if (screenx < 0) screenx = 0;
		for (uint8_t a = 0; a < spritesArraySize; a++) {
			if ((screenx + 128 >= sprites[a].x) && (sprites[a].moving == false)) {
				sprites[a].moving = true;
				sprites[a].vx = -1;
			}
		}
		for (uint8_t a = iObjectIndex; a < iObjectCount; a++) {
			if ((iObject[a].x*8)+8 < screenx) {
				iObjectIndex = a;
				a = iObjectCount;
			}
		}
		drawMap();
		if (sprites[0].y > 64) die();
		ltime = ctime;
	}
	uint8_t input = Arduboy.getInput();	// Bxxlurdab
	if (input & (1<<5)) left = true;
	if (input & (1<<4)) up = true;
	if (input & (1<<3)) right = true;
	if (input & (1<<2)) down = true;
	if (input & (1<<1)) aBut = true;	// a button?
	if (input & (1<<0)) bBut = true;	// b button?
}

