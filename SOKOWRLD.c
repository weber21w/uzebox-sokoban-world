//2010 Lee Weber (D3thAdd3r)
//Released under GPL >= 3.0, do what you will with it.

//Don't have the time to clean this up properly, had to get it done and things went wrong :)
//build options for different episodes is in gamedefines.h, have fun.



#include <avr/pgmspace.h>
#include <string.h>
#include <uzebox.h>
#include <stdint.h>

#include "data/tiles.inc"
#include "data/sprites.inc"
#include "data/frames.inc"
#include "data/patches.inc"
#include "data/music-compressed.inc"

//////////////BUILD CONTROL FOR DIFFERENT EPISODES//////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#define SOKOBAN_EPISODE_NUMBER 0//original episode
//#define SOKOBAN_EPISODE_NUMBER 1  //the first level pack


#if SOKOBAN_EPISODE_NUMBER == 0

	#define SOKOBAN_WORLD_ID 62
	#define EPISODE_SAVE_OFFSET 0
	#include "data/ep1Maps.inc"
	#include "data/ep1Demos.inc"

#else// SOKOBAN_EPISODE_NUMBER == 1

	#define SOKOBAN_WORLD_ID 62//will need to change to 63 for next 2 episodes(room for 2 eps. per block)
	#define EPISODE_SAVE_OFFSET 15//offset into save block for new episode
	#define NOENDING 1//needed to fit large demos
	#define NOINTRO 1//ditto
	#include "data/ep2Maps.inc"
	#include "data/ep2Demos.inc"

#endif

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////




#define TITLEWAITSEC 5

#define TITLEWAITTIME  TITLEWAITSEC*60
#define SCREEN_WIDTH  30*8
#define SCREEN_HEIGHT 28*8



#define NUMTILES 6
#define NUMTILESETS 5
#define NUMTILESPERSET NUMTILES*4
#define NUMSPRITESPERSET 36

#define BLANK  0
#define FLOOR  1
#define TARGET 2
#define BLOCK  3
#define TBLOCK 4
#define WALL	5
#define SOKOSTART 6//not counted

#define LEVELWIDE 15
#define LEVELHIGH 14
#define LEVELSIZE LEVELWIDE*LEVELHIGH
#define LEVELINFOSIZE 1//tile set
#define LEVELTOTALSIZE LEVELSIZE+LEVELINFOSIZE

#define MOVELISTSIZE 64//undo/redo where 4 moves/byte (64=256moves)

u8 spritecount;
u8 tileset;

u8 level;
u8 opentargets;
u8 demo;
/*
u8 movepos,nummoves;
u8 movelist[MOVELISTSIZE];
*/
int moves,pushes;
int optimummoves,optimumpushes;
bool levelcompleted,levelparmet;//whether the level has been completed before and the par met
u8 numlevels;

u8 demonum;
bool demoplaying;

bool musicoff = false;//user wants the music off, don't restart it
bool musicplaying = true;//a song is currently playing
bool musicpreference = false;//user has manually set music, don't change this setting from now on
bool graphicspreference = false;
u8	preferredtrack = 0;

unsigned int padstate,oldpadstate;
#define UP		1
#define DOWN	 2
#define LEFT	 4
#define RIGHT	8
#define MOVE	16
#define PUSH	32

u8 px,py,pstate,poffset;
u8 pframe,pftime;
#define NUMPFRAMES 0
#define PFRAMELENGTH 7

u8 bx,by,bnx,bny;

u8 guistate,gamestate;
bool			 guijuststarted;
u8 cursorpos;
#define GMAINMENU	 1
#define GLEVELSELECT 2
#define GOPTIONS	  3
#define GEDITOR		4
#define GVICTORY	  5
#define GHELP		  6
#define GINGAMEMENU  7

#define SCANNINGOPTIMUM 64//we are only running logic to calculate the optimum moves/pushes for a solution. kind of ugly hack.
#define FASTMOVE 128


u8 tracknum;

u8 outeffectnum,ineffectnum;
u8 menurestore[4];




////////////TODO CLEAN THIS UP!!////////////
extern int abs();
extern u8 ram_tiles[];

void VictoryMenu();
void StripeScreenOut(int speed, bool hrz);
void StripeScreenIn(int speed,  bool hrz);
void BlockScreenOut(int speed);
bool PlayDemo(u8 demonum);
void CalculateOptimum(u8 demonum);
void DoScreenOutEffect();
void DoScreenInEffect();
void ttoram(int toff, int roff, int len);
void storam(int soff, int roff, int len);
void DrawMenu(u8 x, u8 y, u8 w, u8 h);
void DrawOpeningMenu(u8 x, u8 y, u8 w, u8 h, int speed, bool smooth);
void RetryLevel();
void SaveUserMap(u8 slot);
void rotateram(u8 r);

bool HasMetLevelPar(u8 map);
void SetMetLevelPar(u8 map);
void SetLevelCompleted(u8 map);
bool IsLevelCompleted(u8 map);

void SwapTileSet(u8 newset);

static bool SokoMove();
u8 GetMapTile(u8 x, u8 y);

void AmericaEnterance();
void EgyptEnterance();
void ChinaEnterance();
void SouthPoleEnterance();
void JapanEnterance();
void GameEnding();
bool AllLevelsCompleted();

void SetVram(u8 x, u8 y, u8 t){//direct vram, bypass RAM_TILES_COUNT
	vram[(y*30)+x] = t;
}

void printb(u8 x,u8 y, u8 val){
	u8 c = val%10;
	SetTile(x+1,y,FONTSTART+4+c);
	
	c=(val/10);
	if(!c){SetTile(x,y,BLANKT+1);}else{SetTile(x,y,FONTSTART+4+c);}
}



void printi(u8 x,u8 y, int val){
	 if(val > 999)
		val = 999;

	unsigned char c,i;

	for(i=0;i<4;i++){
		c=val%10;
		if(val>0 || i==0){
			SetTile(x--,y,c+FONTSTART+4);
		}else{
			SetTile(x--,y,FONTSTART);
		}
		val=val/10;
	}

}

void printdigitoverlaid(u8 x,u8 y, u8 val, u8 ramoff, u8 ck, u8 bg){
	u8 c = val;//%10;
	 int toff = (FONTSTART+4+c)*64;	
	 int roff = ramoff*64;
	 u8 t;

	 for(u8 i=0;i<64;i++)//copy over the background tile
		ram_tiles[roff+i] = pgm_read_byte(&GameTiles[(bg*64)+i]);

	vram[(y*30)+x] = ramoff;
	 for(u8 i=0;i<64;i++){
		t = pgm_read_byte(&GameTiles[toff]);
		if(t != ck)
			ram_tiles[roff] = t;

		toff++;
		roff++;
	}
}

void print(u8 x,u8 y,const char *string){
	int i=0;
	char c;

	while(true){
		c=pgm_read_byte(&(string[i++]));		
		if(c!=0){
			c=((c&127)-32);		

			SetTile(x++,y,c+FONTSTART-19);
		}else{
			break;
		}
	}	
}



void FillScreen(u8 t);
void ResetSpriteCount();
void DoHideSprites();
static bool PutSprite(u8 x, u8 y, u8 t, bool flip);
void Render();
void DrawMapSection(u8 sx, u8 sw, u8 sm, u8 dx);
void DrawSpriteTile(u8 x, u8 y, u8 f);


void Undo();
void Redo();

void HideSprites(u8 start, u8 end){
	 for(u8 i=start;i<end;i++)
		sprites[i].x = SCREEN_TILES_H*TILE_WIDTH;
}


void ClearSprites(){//hides all sprites, resets sprite count
	HideSprites(0,32);
	//ResetSpriteCount();
}



void SetGuiState(u8 state);
void LoadLevel(bool flushmoves, bool fadein, bool shiftdown, bool skiptoprow, bool fromeep, bool skipsong);

void SetMapTile(u8 x, u8 y, u8 t);
bool IsBlock(u8 t){if(t==BLOCK||t==TBLOCK){return true;}return false;}
bool IsTBlock(u8 t){if(t==TBLOCK){return true;}return false;}
bool IsWall(u8 t){if(t==WALL){return true;}return false;}
bool IsSolid(u8 t){if(IsBlock(t)||IsTBlock(t)||IsWall(t)){return true;}return false;}



#include "ramfx.h"


//these functions return true if the button was just pressed and not held
bool StartDown() {return((padstate  & BTN_START) && !(oldpadstate & BTN_START)) ;}
bool SelectDown(){return((padstate  & BTN_SELECT)&& !(oldpadstate & BTN_SELECT));}
bool UpDown()	 {return((padstate  & BTN_UP)	 && !(oldpadstate & BTN_UP))	;}
bool DownDown()  {return((padstate  & BTN_DOWN)  && !(oldpadstate & BTN_DOWN)) ;}
bool LeftDown()  {return((padstate  & BTN_LEFT)  && !(oldpadstate & BTN_LEFT)) ;}
bool RightDown() {return((padstate  & BTN_RIGHT) && !(oldpadstate & BTN_RIGHT));}

void FillPad(){
	oldpadstate = padstate;
	padstate = ReadJoypad(0);
}

void Input(){
	FillPad();
	
	if(StartDown()) {guijuststarted=true;guistate=GINGAMEMENU;}
	if(SelectDown()){RetryLevel();return;};

/*
	if(padstate & BTN_A && !(oldpadstate & BTN_A))
		Undo();
	if(padstate & BTN_B && !(oldpadstate & BTN_B))
		Redo();
*/


	if(padstate & BTN_SR && !(oldpadstate & BTN_SR)){
		if(gamestate & FASTMOVE){gamestate ^= FASTMOVE;}
	  else						  {gamestate |= FASTMOVE;}
	}

	if(poffset)
		return;

	if(padstate & BTN_UP)		  {pstate = UP	| MOVE;}
	else if(padstate & BTN_DOWN) {pstate = DOWN | MOVE;}
	else if(padstate & BTN_LEFT) {pstate = LEFT | MOVE;}
	else if(padstate & BTN_RIGHT){pstate = RIGHT| MOVE;}
}


void LoadLevel(bool flushmoves, bool fadein,bool shiftdown, bool skiptoprow, bool fromeep, bool skipsong){
	gamestate = 0;//turn off fast flag
	moves = pushes = 0;
	px = py = poffset = opentargets = 0;

	if(level > NUMLEVELS){level = NUMLEVELS-1;}

	levelcompleted = IsLevelCompleted(level);
	levelparmet	 = HasMetLevelPar(level);

//	movepos = 0;
	if(flushmoves){//don't flush moves if this was called from Undo()
/*	  // movepos = 0;
	  nummoves = 0;
	  for(int i=0;i<MOVELISTSIZE;i++)
		  movelist[i] = 0;
*/
	}
	if(fadein){//we have to copy the tiles to ram tiles

	}

	bx = 255;
	pstate = DOWN;
	u8 t;

	int moff = (level*((8*14)+LEVELINFOSIZE));//+(fromeep*SOKOBANSAVEOFFSET);
	u8 defaulttileset;


	defaulttileset = pgm_read_byte(&Maps[moff++]);

	if(!graphicspreference)//user hasn't specified a tileset
		tileset = defaulttileset;

	if(!musicoff && !skipsong){//user hasn't specified no music so see if we should load music
		u8 oldtrack = tracknum;

		if(musicpreference)//user has specified a music track
		  tracknum = preferredtrack;
		else
		  tracknum = defaulttileset+1;//use tile set default
		
	  if(!musicplaying || (oldtrack != tracknum)){//music is different so start a new song, otherwise just let the song continue
	  //note that the victory menu will set tracknum to 255 so that it always loads a song after(unless music off ^)
			
		 if(tracknum == 1)			{StartSong(AmericaSong);}
			else if(tracknum == 2)	 {StartSong(EgyptSong);}
			else if(tracknum == 3)	 {StartSong(ChinaSong);}
			else if(tracknum == 4)	 {StartSong(SouthPoleSong);}
			else /*if(tracknum == 5)*/{StartSong(JapanSong);}	

		 musicplaying = true;
	  }

	}

	for(u8 y=skiptoprow;y<LEVELHIGH;y++)//don't draw over menu
	for(u8 x=0;x<LEVELWIDE;x++){
		if(!fromeep){t = pgm_read_byte(&Maps[moff+(y<<3)+(x>>1)]);}
	  else		  {t = ReadEeprom(moff+(y<<3)+(x>>1));}
	  
	  t = ( t>>((4)*(x%2)) ) & 15; //decompress 4 bits for each tile

	  if(t >= SOKOSTART){
		  px = x;
		 py = y;  
			if(t > SOKOSTART){t=TARGET;}//sokoban starts on a target;
		 else				 {t=FLOOR;}
			
	  }
	  else if(t == TARGET){opentargets++;}
		  SetMapTile(x,y+shiftdown,t);//this is relative to tile set
	}
}

void Logic(){
	if(guijuststarted)
		return;
	
	if(!opentargets && !(gamestate & SCANNINGOPTIMUM)){//victory!
		if(demoplaying)
		  return;

		WaitVsync(5);//let box sfx finish
		VictoryMenu();
	  FadeOut(2,true);

		level++;
	  if(level >= (numlevels-1)){
		  level = numlevels-1;

#ifndef NOENDING
		 if(AllLevelsCompleted())
				GameEnding();
#endif
		 guijuststarted = true;
		 guistate = GLEVELSELECT;
		}
	  else{
			//if(level < FIRSTEGYPTLEVEL)
			// AmericaEnterance();
		 //else 
		 if(level == FIRSTEGYPTLEVEL)
				EgyptEnterance();
			else if(level == FIRSTSOUTHPOLELEVEL)
				SouthPoleEnterance();
			else if(level == FIRSTCHINALEVEL)
				ChinaEnterance();
			else if(level == FIRSTJAPANLEVEL)
				JapanEnterance();
		}

	  WaitVsync(1);
	  LoadLevel(true,true,false,false,false,false);
	  FadeIn(2,false);
	  return;
	}

	if(pftime)
		pftime--;
	else{
		pftime = PFRAMELENGTH;
		pframe = !pframe;
	}
	
	if(pstate & MOVE){
		if(poffset == 0){//user just pressed move, see if it's valid
			if(!SokoMove()){
			 pstate ^= MOVE;
			 if(pstate & PUSH){pstate ^= PUSH;}
			
			return;
		  }
		 
		}

		//if(poffset == 0 || poffset == 8){TriggerFx(SFX_STEP,SFX_STEP_VOL,false);}//play the foot step sound
	  poffset++;
	  if(gamestate & FASTMOVE){poffset++;}
	  if(poffset > 15){//we moved a whole square so do whatever needs to be done
		 poffset = 0;
		 pstate ^= MOVE;

/*
			//Add the move to the undo list
			if(movepos != 255){//is there enough space left in the list 256 moves 64*4
			 if(movepos == nummoves){nummoves++;movepos++;}//make sure there are no moves after this that would be overwritten
			  u8 ts;
			if(pstate & UP)		 {ts=0;}
			else if(pstate & DOWN){ts=1;}
			else if(pstate & LEFT){ts=2;}
			else						{ts=3;}
				
			ts = movelist[movepos/4];
			ts +=  ts<<((movepos%4)*2);//shift 2 bits into position
			
			movelist[movepos/4] = ts;//store 2 bits
			 movepos++;
			printb(movepos,0,ts<<(movepos%4));
			printb(20,0,movepos);
			if(gamestate & FASTMOVE)
				poffset++;
		 }
*/

		 if(pstate & UP)		 {py--;}
		 else if(pstate & DOWN){py++;}
		 else if(pstate & LEFT){px--;}
		 else/*pstate & RIGHT*/{px++;}
			
		 moves++;
		 if(bx != 255){//a block was active during this move
			 pushes++;
			if(GetMapTile(bnx,bny)==TARGET){
				SetMapTile(bnx,bny,TBLOCK);//put the block over target in vram
				if(!(gamestate & SCANNINGOPTIMUM)){TriggerFx(SFX_TARGET,SFX_TARGET_VOL,false);}//play the block place sound
				opentargets--;//one less target left
			}
			else
				SetMapTile(bnx,bny,BLOCK);

				//SetMapTile(bnx,bny,bx=(BLOCK+(GetMapTile(bnx,bny)==TARGET)));
			 //if(bx==TBLOCK){opentargets--;}
		 }
		 bx = 255;//deactivate the block
	  }
	}
	else//pstate & MOVE
		if(pstate & PUSH){pstate ^= PUSH;}

}

bool SokoMove(){
	u8 t,t2;//t = block player moves to, t2 = block 1 past t(where a pushed block would go)
	u8 x,x2;//same thing for x values
	u8 y,y2;//ditto
	x = x2 = px;//set defaults
	y = y2 = py;

	if(pstate & UP){
		if(py == 0){return false;}//don't let player move off the screen
	  t = GetMapTile(px,py-1);
		if(py-1 == 0){t2 = FLOOR;}//don't check outside of map just let the move happen
		else			{t2 = GetMapTile(px,py-2);}
		if(py-1 == 0 && IsBlock(t)){return false;}//don't let player push a block off the map
	  y  = py-1;
	  y2 = py-2;
	}
	else if(pstate & DOWN){
		if(py == 13){return false;}//don't let player move off the screen
	  t = GetMapTile(px,py+1);
		if(py+1 == 27){t2 = FLOOR;}//don't check outside of map just let the move happen
		else			 {t2 = GetMapTile(px,py+2);}
		if(py+1 == LEVELHIGH-1 && IsBlock(t)){return false;}//don't let player push a block off the map
	  y  = py+1;
	  y2 = py+2;
	}
	else if(pstate & LEFT){
		if(px == 0){return false;}//don't let player move off the screen
	  t = GetMapTile(px-1,py);
		if(px-1 == 0){t2 = FLOOR;}//don't check outside of map just let the move happen
		else			{t2 = GetMapTile(px-2,py);}
		if(px-1 == 0 && IsBlock(t)){return false;}//don't let player push a block off the map
	  x  = px-1;
	  x2 = px-2;
	}
	else/*if(pstate & RIGHT)*/{
		if(px == 14){return false;}//don't let player move off the screen
	  t = GetMapTile(px+1,py);
		if(px+1 == 29){t2 = FLOOR;}//don't check outside of map just let the move happen
		else			 {t2 = GetMapTile(px+2,py);}
		if(px+1 == LEVELWIDE-1 && IsBlock(t)){return false;}//don't let player push a block off the map
	  x  = px+1;
	  x2 = px+2;
	}

	//ok, we know what the player is moving into and what is beyond that(if relevant). piece of cake now.
	if(IsBlock(t)){//we're pushing a block or a block on a target
		if(IsSolid(t2))//is something solid behind it?
		  return false;
		//if we're here then the move will suceed, check special case block on a target

	  if(IsTBlock(t)){SetMapTile(x,y,TARGET);opentargets++;}//draw the target underneath and update number of targets uncovered
	  else			  {SetMapTile(x,y,FLOOR);}//draw the floor underneath
	  bx = x;//set up the block for rendering
	  by = y;
	  bnx = x2;
	  bny = y2;
	  pstate |= PUSH;
	}
	else if(IsSolid(t)){
		if(pstate & PUSH){pstate ^= PUSH;}
		return false;
	}
	return true;
}

u8 GetMapTile(u8 x, u8 y){
	u8 t = vram[((y<<1)*30)+(x<<1)]-RAM_TILES_COUNT;
	t = t>>2;//adjust it for tile size
	t -= tileset*NUMTILES;//adjust for tile set
	return t;	
}

void RetryLevel(){
	//DoScreenOutEffect();
	FadeOut(3,true);
	WaitVsync(1);
	LoadLevel(true,true,false,false,false,true);
	WaitVsync(1);
	Render();//make sure player gets drawn once so he doesn't "warp" to place
	FadeIn(3,false);
}


/*
void Redo(){
	if(movepos >= nummoves){return;}//nothing to redo	

	u8 ts = (( movelist[movepos/4]>>((movepos%4)*2) ) & 3);//get 2 bits from the current move list position

	if(ts == 0)		{pstate = UP|MOVE;}
	else if(ts == 1) {pstate = DOWN|MOVE;}
	else if(ts == 2) {pstate = LEFT|MOVE;}
	else				 {pstate = RIGHT|MOVE;}

	poffset = 0;	Logic();//let logic setup any blocks
	poffset = 16;  Logic();//let logic finish a move
	//movepos++;// done in logic
}


void Undo(){//do this the easy way
	if(nummoves == 0 || nummoves == 254){return;}//nothing to undo or out of space
	u8 nm = nummoves-1;
	u8 count = 0;
	LoadLevel(false,false,false,false,false);//reload level, don't clear move list
	//movepos = 0;//already done in load level

	while(movepos < nm){//just step back through
		//if(++count > 20){WaitVsync(1);count = 0;}//give the kernel some time...

	  Redo();
	}
	WaitVsync(1);
}
*/

bool IsLevelCompleted(u8 map){
	struct EepromBlockStruct ebs;
	ebs.id = SOKOBAN_WORLD_ID;

	if(EepromReadBlock(ebs.id, &ebs) == 0)
		return (ebs.data[(map/8)+EPISODE_SAVE_OFFSET]>>(map%8))&1;//decode 1 bit from eeprom
	else{
		for(u8 i=0;i<30;i++)
		  ebs.data[i] = 0;//make sure the data here is formatted

	  EepromWriteBlock(&ebs);//make a new one then
	}
	return false;

}

void SetLevelCompleted(u8 map){

	struct EepromBlockStruct ebs;
	ebs.id = SOKOBAN_WORLD_ID;

	if(EepromReadBlock(ebs.id, &ebs) != 0)//no save game
		EepromWriteBlock(&ebs);//make a new one then

		u8 t = (u8)(ebs.data[(map/8)+EPISODE_SAVE_OFFSET]);		
		
	  t |= (1<<(map%8));
	  
	  ebs.data[(map/8)+EPISODE_SAVE_OFFSET] = t;
	  
		EepromWriteBlock(&ebs);
	
}


void SetMetLevelPar(u8 map){
	//To save some code space, well simply store this like level complete progress.
	//We will offset the data by 56 levels(7 bytes) from the start of level completed progress.
	//Maximum 56 levels per pack, can store save progress for 2 episodes in one block
	SetLevelCompleted(map+56);
}

bool HasMetLevelPar(u8 map){//Same concept as above
	return IsLevelCompleted(map+56);
}

bool AllLevelsCompleted(){
	for(u8 i=0;i<NUMLEVELS;i++){
		if(!IsLevelCompleted(i))
		  return false;
	}
	return true;
}


void StartMusicTrack(u8 track){
	
	if(track == 1)	  {StartSong(AmericaSong);}
	else if(track == 2){StartSong(EgyptSong);}
	else if(track == 3){StartSong(ChinaSong);}
	else if(track == 4){StartSong(SouthPoleSong);}
	else if(track == 5){StartSong(JapanSong);}
	
}


void GuiScreenFlush();
//This whole gui is a mess. Enjoy...

void SetGuiState(u8 state){
	guijuststarted = true;
	guistate = state;
}

void AttractMode(){
	if(demonum==NUMDEMOS){demonum = 0;}
	PlayDemo(demonum++);
}

void DrawMenu(u8 x, u8 y, u8 w, u8 h){
	int off = (y*30)+x;

	u8 t;
	t = vram[off+0]-RAM_TILES_COUNT;
	if(t < MENUSTART || t == BLANKT){menurestore[0] = vram[off+ 0]-RAM_TILES_COUNT;}//save the tiles under the corners if they aren't already menu tiles
	t = vram[off+w]-RAM_TILES_COUNT;
	if(t < MENUSTART || t == BLANKT){menurestore[1] = vram[off+ w]-RAM_TILES_COUNT;}
	
	off += 30*h;
	
	t = vram[off+0]-RAM_TILES_COUNT;
	if(t < MENUSTART || t == BLANKT){menurestore[2] = vram[off+ 0]-RAM_TILES_COUNT;}
	t = vram[off+w]-RAM_TILES_COUNT;
	if(t < MENUSTART || t == BLANKT){menurestore[3] = vram[off+ w]-RAM_TILES_COUNT;}

	SetTile(x+0,y+0,MENUSTART+5);//draw the corners
	SetTile(x+w,y+0,MENUSTART+6);
	SetTile(x+0,y+h,MENUSTART+7);
	SetTile(x+w,y+h,MENUSTART+8);

	for(u8 i=x+1;i<x+w;i++){SetTile(i,y,MENUSTART+0);SetTile(i,y+h,MENUSTART+1);}//draw top and bottom
	for(u8 i=y+1;i<y+h;i++){SetTile(x,i,MENUSTART+2);SetTile(x+w,i,MENUSTART+3);}

	for(u8 i=y+1;i<y+h;i++)
	for(u8 j=x+1;j<x+w;j++)
		SetTile(j,i,FONTSTART);
	
}


void SmoothMenuCorners(u8 x,u8 y,u8 w,u8 h,u8 roff){//get rid of the black pixels on menu corners
  
	//TODO - SPRITES ARE WAY MORE EFFICIENT
	ttoram((MENUSTART+5)*64,(26*64),(4*64));//copy over the 4 ram tiles starting from ram tile 26
//	u8 t;
	int off = (y*30)+x;//offset to top left piece of menu

	vram[off +0] = roff;//set the ram tile we pre rendered
	replaceramcolor(menurestore[0],roff++,0);//replace the color black with the colors from the tile that was underneath

	vram[off+ w] = roff;
	replaceramcolor(menurestore[1],roff++,0);

	off += (h*30);

	vram[off+ 0] = roff;
	replaceramcolor(menurestore[2],roff++,0);

	vram[off+ w] = roff;
	replaceramcolor(menurestore[3],roff++,0);

}


void DrawOpeningMenu(u8 x, u8 y, u8 w, u8 h, int speed, bool smooth){
	u8 mt;
	u8 count;
	mt = 1;
	count = speed;
	while(mt<w+1){//open horizontally
		DrawMenu(x,y,mt,1);
	  if(smooth){SmoothMenuCorners(x,y,mt,1,26);}
	  if(speed < 0){
			WaitVsync(abs(speed));
	  }
	  else{
		  if(!count){WaitVsync(1);count=speed;}else{count--;}
	  }
	  mt++;
	}
	mt = 1;
	count = speed;
	while(mt<h+1){
		DrawMenu(x,y,w,mt);
	  if(smooth){SmoothMenuCorners(x,y,w,mt,26);}
	  if(speed < 0){
			WaitVsync(abs(speed));
	  }
	  else{
		  if(!count){WaitVsync(1);count=speed;}else{count--;}
	  }
	  mt++;
	}
}

void FillScreen(u8 t){
	for(u8 i=0;i<30;i++)
	for(u8 j=0;j<28;j++)
		SetTile(i,j,t);
}

void GuiScreenFlush(){
	guijuststarted = false;
	WaitVsync(1);
	FillScreen(2);
	ClearSprites();
	FadeOut(0,true);
}

void CloseGui(){
	guistate = 0;
	guijuststarted = 0;
	WaitVsync(1);
	SetTileTable(GameTiles);
	FadeIn(5,false);
}

static void DrawRamTileLine(int xoff, int roff, int doff){
  roff = 0;
  doff = 1;
	while(roff < (10*64)){//draw the top line of ten ascending ram tiles
		if((roff>0) && ((roff&7) == 0)){roff+=(64-8);}//hit the next row, so get to the first x pixel of the next tile
		if((doff>0) && ((doff&7) == 0)){doff+=(64-8);}
		  ram_tiles[doff] = ram_tiles[roff];//pgm_read_byte(&Title[doff++]);
		 roff++;
	}


}



void VramFillWindow(u8 x, u8 y, u8 w, u8 h, u8 rt){
	int off = 0;
	int roff = rt*64;
	for(u8 j=y;j<y+h;j++)
	for(u8 i=x;i<x+w;i++)
		vram[(j*30)+i+roff] = off++;
}

void LinearWindow(u8 x, u8 y, u8 w, u8 h, int t){
	for(u8 j=y;j<y+h;j++)
	for(u8 i=x;i<x+w;i++){
		vram[(j*30)+i] = RAM_TILES_COUNT+t;
		t++;
	}
}

void MainMenu(){
	static bool firstrun = false;
	int idleticks = 0;
	cursorpos = 0;
	tileset = 0;
	u8 t;
	
	graphicspreference = false;
	musicpreference = false;

	HideSprites(0,MAX_SPRITES);
	FillScreen(BLANKT);
	WaitVsync(1);
	FadeIn(2,false);

	if(!firstrun){
		StartSong(AmericaSong);
	  musicplaying = true;
	  tracknum = 1;
		firstrun = true;
	}

	WaitVsync(1);

	HideSprites(0,MAX_SPRITES);
	
	bool flop = true;

	u8 flopcount = 4;
	for(u8 yoff = 22;yoff>2;yoff--){

		for(u8 y=0;y<4;y++){
			for(u8 x=0;x<14;x++){
				t = pgm_read_byte(&TitleMap[((y)*14)+x]);
			  vram[((y+yoff)*30)+x+8] = t+169+RAM_TILES_COUNT;//TODO define 169
			}
		}

		spritecount = 0;
	  tileset = 0;
	  u8 yo = yoff<<3;
	  for(u8 i=0;i<5;i++){//pretty time tight, but it makes it(I think/hope). Lack of cleverness here...
		 switch(i){
			 case 0:
				DrawSpriteTile(64,yo+16,3+flop);
			 break;
				
			case 1:
				DrawSpriteTile(96,yo+32,4-flop);
			break;

				case 2:
				DrawSpriteTile(112,yo+32,3+flop);//Out of time now, because of channel 4 turned on
			break;

				case 3:
				DrawSpriteTile(128,yo+32,4-flop);
			break;

			case 4:
				DrawSpriteTile(160,yo+16,3+flop);
			break;
		  };

		 //DrawSpriteTile((i<<4)+80-((i==0)<<4)+((i==4)<<4),(yoff<<3)+32-((i==0 || i==4)<<4),3+flop);
		  ++tileset;
	  }

		WaitVsync(1);
		if(yoff > 2){
		 int off = ((yoff+4)*30)+10;
		 for(u8 x=10;x<20;x++)
			 vram[off++] = BLANKT+RAM_TILES_COUNT;
	  }

	  flopcount--;
		if(!flopcount){
		  flop = !flop;
		 flopcount = 4;
	  }
	  WaitVsync(1);
	}

	WaitVsync(20);

	DrawOpeningMenu(20,20,9,3,-4,false);
	print(21,21,PSTR("NEW3GAME"));
	print(21,22,PSTR("CONTINUE"));
	WaitVsync(40);

	  while(true){
		FillPad();

		if(UpDown() || DownDown()){cursorpos = !cursorpos;idleticks=0;}
	  else if(StartDown()){
		 // TriggerFx(20,255,true);
		  guijuststarted = true;
		 guistate = GLEVELSELECT;
		 level = 0;
			if(cursorpos == 1){//continue
			 while(IsLevelCompleted(level))
				level++;
				
			if(level >= numlevels)
				level = numlevels-1;
		 }
		 //else{guistate=GMAINMENU;}//guistate = GEDITOR;}
		 //else{guistate = GHELP;}
		 FadeOut(1,true);
		 HideSprites(0,MAX_SPRITES); 
		 return;
	  }
		
	  for(u8 i=0;i<2;i++){SetTile(20,21+i,MENUSTART+2);} SetTile(20,21+cursorpos,MENUSTART+4);//overdraw side of window then draw cursor


	  if(++idleticks > TITLEWAITTIME){
		 FadeOut(1,true);
		 HideSprites(0,MAX_SPRITES);
		 guijuststarted = false;
		  AttractMode();
		 guijuststarted = true;
		 return;
	  }
		
		WaitVsync(1);
		
	  spritecount = 0;
	  if(++flopcount>7){flop = !flop; flopcount = 0;}
		for(u8 i=0;i<5;i++){
		 // if(i == 2){continue;}//not enough time for all 5 now(channel 4)
		  tileset = i;
		 DrawSpriteTile((i<<4)+80-(16*(i==0))+((i==4)<<4),56-((i==0 || i==4)<<4),5+flop);

	  }

		WaitVsync(1);
	}
	
}


void InGameMenu(){
	cursorpos = 0;
	u8 oldgfx,oldtrack;
	oldgfx = tileset;
	oldtrack = tracknum;
	u8 gfxnum = tileset;
	//u8 restore[16];
	int off = 14*64;//0;
	for(u8 y=5;y<10;y++)
	for(u8 x=5;x<10;x++)
		ram_tiles[off++] = GetMapTile(x,y);//restore[off++] = GetMapTile(x,y);
	
	HideSprites(0,MAX_SPRITES);
	DrawOpeningMenu  (10,10,8,8,0,true);
	WaitVsync(1);
	print(11,11,PSTR("BACK"));//strGameMenu1);
	print(11,12,PSTR("RETRY"));
	print(11,13,PSTR("GIVEUP"));
	print(11,14,PSTR("SOLVE"));
	print(11,15,PSTR("BGM"));
	print(11,16,PSTR("GFX"));
	print(11,17,PSTR("QUIT"));
	//now replace those ugly blocky corners with rounded ram tiles
	//...
	while(true){
		FillPad();
		if(UpDown() && cursorpos > 0)	  {cursorpos--;}
	  else if(DownDown() && cursorpos < 6){cursorpos++;}
	  else if(StartDown()){
	  //time to restore vram
		  off = 14*64;//0;
			for(u8 y=5;y<10;y++)
			for(u8 x=5;x<10;x++)
			 SetMapTile(x,y,ram_tiles[off++]);//restore[off++]);
			
		 guijuststarted = false;
		 guistate = 0;
		 if(cursorpos == 0){//back
			 tracknum = oldtrack;
			tileset = oldgfx;
		 }
		 else if(cursorpos == 1){//retry
			  RetryLevel();
		 }
		 else if(cursorpos == 2){//give up
			DoScreenOutEffect();
			guijuststarted = true;
			  guistate = GLEVELSELECT;
		 }
		 else if(cursorpos == 3){//solve
			  // DoScreenOutEffect();
			PlayDemo(level);
			HideSprites(0,MAX_SPRITES);
			guijuststarted = true;
			guistate = GLEVELSELECT;
		 }
		 else if(cursorpos == 4){//bgm
			 StopSong();
			WaitVsync(2);//seems like songs are messed up somehow

			musicplaying = true;

				musicpreference = true;
			preferredtrack = tracknum;

			if(musicoff && (tracknum == tileset+1)){//user has selected the default song, so now lets the music be default again
					musicoff = false;
				musicpreference = false;
				musicplaying = true;
			}

			if(tracknum == 1)	  {StartSong(AmericaSong);}
				else if(tracknum == 2){StartSong(EgyptSong);}
				else if(tracknum == 3){StartSong(ChinaSong);}
				else if(tracknum == 4){StartSong(SouthPoleSong);}
				else if(tracknum == 5){StartSong(JapanSong);}
			else						{musicplaying=false;musicoff=true;musicpreference=false;}

		 }
		 else if(cursorpos == 5){//gfx
			 graphicspreference = true;
				if(gfxnum != tileset){
				Render();
				SwapTileSet(gfxnum);
				 graphicspreference = true;
			}
			else
				graphicspreference = false;//allow user to use default tiles again
		 }
		 else{//quit
				guistate = GMAINMENU;
			guijuststarted = true;
			DoScreenOutEffect();//there is a strange bug here, apply band-aid
			HideSprites(0,MAX_SPRITES);
			FillScreen(BLANKT);
		 }

		 if(cursorpos < 4 && cursorpos > 5){
				tileset = oldgfx;
			tracknum = oldtrack;
		 }
		 return;
	  }
	  if(cursorpos == 4){//BGM
		 if(tracknum > 0 && LeftDown()) {tracknum--;}
		 else if(tracknum < 5 && RightDown()){tracknum++;}
		}
	  else if(cursorpos == 5){//GFX
		  if(gfxnum > 0 && LeftDown()) {gfxnum--;}
		else if(gfxnum < 4 && RightDown()){gfxnum++;}
	  }
	  if(tracknum == 0 || tracknum == 255){print(15,15,PSTR("OFF"));}else{SetTile(16,15,FONTSTART);if(tracknum==pgm_read_byte(&Maps[((level<<3)*14)+level])+1){SetTile(17,15,FONTSTART+2);}else{SetTile(17,15,FONTSTART);}printb(15,15,tracknum);}
	  printb(15,16,gfxnum+1); if(pgm_read_byte(&Maps[((level<<3)*14)+level])==gfxnum){SetTile(17,16,FONTSTART+2);}else{SetTile(17,16,FONTSTART);}
	  for(u8 i=0;i<7;i++){SetTile(10,11+i,MENUSTART+2);}//blank out the cursor
	  SetTile(10,11+cursorpos,MENUSTART+4);
	  WaitVsync(1);
	}
};


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////


void VictoryMenu(){
	StopSong();

	poffset = px*16;//move the player off the screen
	px = 0;
	bx = 255;
	pstate = RIGHT;	
	guijuststarted = false;
	while(poffset < (28*8)){
		poffset += 2;
		if(pftime){pftime--;}else{pftime = 4;pframe=!pframe;}
	  Render();
	}
	HideSprites(0,MAX_SPRITES);

	StartSong(LevelClearSong);
	musicplaying = false;//make sure this is reloaded if necessary inside LoadLevel
	
	WaitVsync(10);
	px = 14;
	py = 6;
	poffset = 0;
	pstate  = LEFT;

	DoScreenOutEffect();
	WaitVsync(1);
	FillScreen(BLANKT);
	FadeOut(1,true);//go blank so we avoid graphical artifacts
	

	int m,p;//save moves/pushes
	m = moves;
	p = pushes;

	//calculate and store achievments
	CalculateOptimum(level);
	FillScreen(BLANKT);
	musicplaying = false;//shouldn't be here

	SetLevelCompleted(level);
	if(m <= optimummoves)
		SetMetLevelPar(level);

	FadeIn(1,true);

	DrawOpeningMenu(9,15,11,10,-2,false);

		print (10,16,PSTR("MOVES")); if(m <= optimummoves) {print (16,16,PSTR("5"));}
		print (10,17,PSTR("PUSHES"));if(p <= optimumpushes){print (16,17,PSTR("5"));}
		printi(19,16,m);
		printi(19,17,p);
		
	  if(m <= optimummoves && p <= optimumpushes){print(11,19,PSTR("GREAT444"));}
	  else													{print(13,19,PSTR("OK44"));}

		print (11,21,PSTR("CPU3BEST"));
		print (10,23,PSTR("MOVES35"));
		print (10,24,PSTR("PUSHES5"));
		printi(19,23,optimummoves);
		printi(19,24,optimumpushes);
	
	WaitVsync(1);

	u8 lvl = level+1;

	u8 banneroff = 29;
	for(u8 i=0;i<90;i++){
		for(u8 j=0;j<30;j++){vram[(12*30)+j]=BLANKT+RAM_TILES_COUNT;}//clear the blur

		printrainbow(banneroff,12,PSTR("STAGE3333CLEARED"),0,0xC0,0x00,i%5);//printrainbow(6,12,strStageCleared,0,0xC0,0x00,i%5);

		for(u8 j=0;j<30;j++){vram[(13*30)+j]=BLANKT+RAM_TILES_COUNT;}//TODO ugly hack for print rainbow messing up??

		if(banneroff < 30-9){
		  if(lvl>9){printdigitoverlaid(banneroff+6,12,lvl/10,21,0xC0,BLANKT);}//printdigitoverlaid(13,12,lvl/10,21,0xC0,BLANKT);}
		  printdigitoverlaid(banneroff+7,12,lvl%10,20,0xC0,BLANKT);
	  }

	  if(banneroff > 7)
		  banneroff--;
		  

	  for(u8 i=0;i<6;i++){
		  FillPad();
		  if(StartDown()){
			  StopSong();
			WaitVsync(1);
			return;
			}

  
			 WaitVsync(1);
	  }
	}
	StopSong();
	WaitVsync(1);
}


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////


void LevelSelect(){
	WaitVsync(1);
	FadeIn(1,false);
	FillScreen(BLANKT);
	WaitVsync(1);
	u8 mappos = level;
	u8 coff = 0;
	u8 cwait = 30;
	bool load = false;
	LoadLevel(true,false,false,false,false,true);

	DrawOpeningMenu(0,0,29,2,0,false);
		 if(levelcompleted){print(26,1,PSTR("5"));}else{print(26,1,PSTR("6"));}
		 if(levelparmet)		 {print(27,1,PSTR("5"));}else{print(27,1,PSTR("6"));}

	while(true){
		FillPad();
	  if	  (padstate & BTN_SL && !(oldpadstate & BTN_SL) && mappos > 0)		  {load=true;if(mappos < 10){mappos = 0;}else{mappos-=10;}}
	  else if(padstate & BTN_SR && !(oldpadstate & BTN_SR) && mappos < numlevels-1){load=true;mappos+=10;if(mappos >= numlevels){mappos = numlevels-1;}}
		//if(UpDown() && cursorpos > 0)		 {cursorpos--;}
	  //else if(DownDown() && cursorpos < 3){cursorpos++;}
	  else if(StartDown()){
			guijuststarted = false;
		 guistate = 0;
		 level = mappos;
		 LoadLevel(true,true,false,false,false,false);

		 return;
	  }
	  else if(LeftDown() && mappos > 0)			{mappos--;load=true;}
	  else if(RightDown() && mappos < NUMLEVELS-1){mappos++;load=true;}

		if(load){
		 DrawMenu(0,0,29,2);
		  DoScreenOutEffect();
		  WaitVsync(1);
		 load = false;
		 level = mappos;
		 LoadLevel(true,true,false,true,false,true);
			DrawMenu(0,0,29,2);
		 if(levelcompleted){print(26,1,PSTR("5"));}else{print(26,1,PSTR("6"));}
		 if(levelparmet)		 {print(27,1,PSTR("5"));}else{print(27,1,PSTR("6"));}
		 //ComposeRamTile((NUMTILESPERSET*tileset)*64,(tileset*28)+4,(25*64));
		 //WaitVsync(1);
		 VramFillWindow(px*2,py*2,2,2,25);
		 // printrainbow(2,1,strLevelSelect,4,0,0,coff);
		 //printb(10+7,1,mappos+1);
		  WaitVsync(1);
	  }
	  if(!cwait){
		  cwait = 30;
	  //	printrainbow(9,1,strLevelSelect,4,0,0,coff++);
		 
		 if(tileset==0)	  {printrainbow(6,1,PSTR("STAGE3333AMERICA")  ,11,0,0,coff++);}
		 else if(tileset==1){printrainbow(6,1,PSTR("STAGE3333EGYPT")	 ,11,0,0,coff++);}
		 else if(tileset==2){printrainbow(6,1,PSTR("STAGE3333CHINA")	 ,11,0,0,coff++);}
		 else if(tileset==3){printrainbow(6,1,PSTR("STAGE3333SOUTHPOLE"),11,0,0,coff++);}
		 else					{printrainbow(6,1,PSTR("STAGE3333JAPAN")	 ,11,0,0,coff++);}
		 printb(6+6,1,mappos+1);
			if(coff > 3){coff = 0;}
	  }
	  else
		  cwait--;

		spritecount = 0;
		DrawSpriteTile((px*16),(py*16),5+(cwait>14));
	  WaitVsync(1);
	}
}


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

void DoScreenOutEffect(){
	WaitVsync(1);
	HideSprites(0,MAX_SPRITES);
	RamifyTileSet();
	RamifyScreen();

	int off = outeffectnum*6;
	u8 parms[6];

	for(u8 i=0;i<6;i++)
		parms[i] = pgm_read_byte(&ScreenFadeParms[off++]);
	
	WaitVsync(1);
	BitMaskScreenOut(parms[0], parms[1], parms[2], parms[3], parms[4], parms[5]);


	if((++outeffectnum)>=(NUMOUTEFFECTS)){outeffectnum=0;}
}



void DoScreenInEffect(){
	if(++ineffectnum>NUMINEFFECTS){ineffectnum=0;}
}

void AmericaEnterance(){}

void EgyptEnterance(){}


void SouthPoleEnterance(){}

void ChinaEnterance(){}

void JapanEnterance(){}

void ColorSection(u8 x, u8 y, u8 w, u8 h, u8 c, u8 rt){//fill a section with color
	for(u8 j=y;j<y+h+1;j++){
		for(u8 i=x;i<x+w+1;i++){//first set up the section with the ram tile index
			vram[(j*30)+i] = rt;
		  }
	}
	int off = rt<<6;//get offset to the ram tile we are using
	for(u8 i=0;i<64;i++){ram_tiles[off++] = c;}
}

const char strIntro1[] PROGMEM = "UZEBOX";

void GameIntro(){
#ifndef NOINTRO
	FillScreen(FONTSTART-1);//ColorSection(0,0,29,27,255,29);
	FadeIn(2,false);
	
	for(u8 i=0;i<3;i++){
		printramtilesfancy(13,14,strIntro1,22,0xFF,8-i,0xC0,0);WaitVsync(10);
	}
 
	
	WaitVsync(45);

	for(u8 i=4;i<10;i++){
		printramtilesfancy(13,14,strIntro1,22,0xFF,8-i,0xC0,0);WaitVsync(10);
	}

	
	WaitVsync(60);
	FadeOut(3,true);
	FadeIn(1,false);
#endif
}





#ifndef NOENDING
const char strEnd1[] PROGMEM = "WOW44333YOU3DID3IT44";  
const char strEnd2[] PROGMEM = "IF3YOU3DID3NOT3USE3EMUZE"; 
const char strEnd3[] PROGMEM = "THEN3GREAT3JOB444"; 
const char strEnd4[] PROGMEM = "YOU3ARE3LIKELY3THE3ONLY"; 
const char strEnd5[] PROGMEM = "PERSON3TO3EVER3SEE3THIS4";//even now that the source is released :) 
const char strEnd6[] PROGMEM = "CONSIDER3YOURSELF3THE3BEST";
const char strEnd7[] PROGMEM = "SOKOBAN3WORLD3PLAYER3EVER4";  
const char strEnd8[] PROGMEM = "THANKS3FOR3PLAYING3ALL3THE";
const char strEnd9[] PROGMEM = "WAY3THROUGH43SERIOUSLY3THANKS";
const char strEnd10[] PROGMEM = "SORRY3FOR3THE3LAME3ENDING";
const char strEnd11[] PROGMEM = "I3AM3JUST3OUT3OF3TIME3NOW";
const char strEnd12[] PROGMEM = "CREATED3BY3LEE3WEBER";
//const char strWasteSpace[] PROGMEM = "5WASTE3THE3SPACE33";
#endif

void GameEnding(){
#ifndef NOENDING
	FadeIn(3,false);
	//print(0,0,strWasteSpace);
	//print(1,0,strWasteMore);
	FillScreen(BLANKT+1);
	WaitVsync(1);
	
	for(u8 i=0;i<sizeof(strEnd1)-1;i++){
		SetTile(6+i,4,(pgm_read_byte(&strEnd1[i])&127)+FONTSTART-51);
	  WaitVsync(8);
	}

	for(u8 i=0;i<sizeof(strEnd2)-1;i++){
		SetTile(4+i,5,(pgm_read_byte(&strEnd2[i])&127)+FONTSTART-51);
	  WaitVsync(8);
	}

	for(u8 i=0;i<sizeof(strEnd3)-1;i++){
		SetTile(8+i,6,(pgm_read_byte(&strEnd3[i])&127)+FONTSTART-51);
	  WaitVsync(8);
	}

	for(u8 i=0;i<sizeof(strEnd4)-1;i++){
		SetTile(4+i,8,(pgm_read_byte(&strEnd4[i])&127)+FONTSTART-51);
	  WaitVsync(8);
	}

	for(u8 i=0;i<sizeof(strEnd5)-1;i++){
		SetTile(4+i,9,(pgm_read_byte(&strEnd5[i])&127)+FONTSTART-51);
	  WaitVsync(8);
	}

	for(u8 i=0;i<sizeof(strEnd6)-1;i++){
		SetTile(3+i,12,(pgm_read_byte(&strEnd6[i])&127)+FONTSTART-51);
	  WaitVsync(8);
	}

	for(u8 i=0;i<sizeof(strEnd7)-1;i++){
		SetTile(3+i,13,(pgm_read_byte(&strEnd7[i])&127)+FONTSTART-51);
	  WaitVsync(8);
	}

	WaitVsync(60);

	for(u8 i=0;i<sizeof(strEnd8)-1;i++){
		SetTile(3+i,15,(pgm_read_byte(&strEnd8[i])&127)+FONTSTART-51);
	  WaitVsync(8);
	}

	for(u8 i=0;i<sizeof(strEnd9)-1;i++){
		SetTile(1+i,16,(pgm_read_byte(&strEnd9[i])&127)+FONTSTART-51);
	  WaitVsync(8);
	}

	for(u8 i=0;i<sizeof(strEnd10)-1;i++){
		SetTile(3+i,18,(pgm_read_byte(&strEnd10[i])&127)+FONTSTART-51);
	  WaitVsync(8);
	}

	for(u8 i=0;i<sizeof(strEnd11)-1;i++){
		SetTile(3+i,19,(pgm_read_byte(&strEnd11[i])&127)+FONTSTART-51);
	  WaitVsync(8);
	}

	print(4,24,strEnd12);
	WaitVsync(255);
#endif
}

void Gui(){
	if(!guistate)
		return;
	switch(guistate){
	case GMAINMENU:
	MainMenu();
	break;
	case GLEVELSELECT:
	LevelSelect();
	break;
//	case GEDITOR:
//	Editor();
//	break;
	case GVICTORY:
	VictoryMenu();
	break;
	case GINGAMEMENU:
	InGameMenu();
	break;
	}
}

void Render(){
	if(guijuststarted){return;}
	char xoff = 0;
	char yoff = 0;
	u8 foff = 0;
	if(pstate & UP)		 {yoff=-poffset;foff= 1;}
	else if(pstate & DOWN){yoff= poffset;foff= 5;}
	else if(pstate & LEFT){xoff=-poffset;foff= 9;}
	else/*pstate & RIGHT*/{xoff= poffset;foff=13;}
	foff += pframe;
	if(pstate & PUSH){foff+=2;}
	spritecount = 0;
	DrawSpriteTile((px<<4)+xoff,(py<<4)+yoff,foff);
	if(bx != 255){DrawSpriteTile((bx<<4)+xoff,(by<<4)+yoff,0);}
	HideSprites(spritecount,MAX_SPRITES);
	WaitVsync(1);
}

void DrawMapSection(u8 sx, u8 sw, u8 sm, u8 dx){
	int moff = sm*LEVELTOTALSIZE;
	moff += sx;
	for(u8 x=0;x<sw;x++){
		for(u8 y=0;y<14;y++){
			SetMapTile(x,y,pgm_read_byte(&Maps[moff]));
		  moff += 15;
		}
		++moff;
	}
}

void SetMapTile(u8 x, u8 y, u8 t){//this is relative to tileset
	int off = ((y<<1)*30)+(x<<1);
	t = (tileset*(NUMTILES<<2)) + (t<<2) + RAM_TILES_COUNT;
	
	vram[off+ 0] = t++;
	vram[off+ 1] = t++;
	vram[off+30] = t++;
	vram[off+31] = t;
}

void SwapTileSet(u8 newset){
	WaitVsync(1);
	u8 oldset,t;
	oldset = tileset;
	for(u8 y=0;y<14;y++){
		for(u8 x=0;x<15;x++){
			tileset = oldset;
		  t = GetMapTile(x,y);
			tileset = newset;
		  SetMapTile(x,y,t);
		}
		if(y == py){
		  Render();
		  WaitVsync(1);
		}
		else
			WaitVsync(2);
	}

	tileset = newset;
}

void DrawSpriteTile(u8 x, u8 y, u8 f){//not optimized, probably ok
	int t,m;
	int toff = tileset*SPRITESPERTILESET ;
	int  fo = f*5;
	int mo = (pgm_read_byte(&FrameTable[fo+4]))*4;
	int to = 0;
	
	for(u8 j=0;j<2;j++){
	for(u8 i=0;i<2;i++){
		t = pgm_read_byte( &FrameTable[fo+to]);
		m = pgm_read_byte(&MirrorTable[mo+to]);
		PutSprite(x,y,t+toff,m);
		x += 8;
	  to++;
	}
		x -= 16;
		y += 8;
	}
}

bool PutSprite(u8 x, u8 y, u8 t, bool flip){
	//if(spritecount >= MAX_SPRITES){return false;}

	sprites[spritecount].x=x; sprites[spritecount].y=y; sprites[spritecount].tileIndex=t;
	sprites[spritecount].flags = SPR_FLIP_X*flip;
	
	spritecount++;
	return true;
}



void DemoSetup();
void DemoShutdown();

bool DemoQuit(){
	demoplaying=false;guijuststarted=true;guistate=GMAINMENU;//return true;
	DoScreenOutEffect();
	WaitVsync(1);
	return true;
}

bool PlayDemo(u8 demonum){//TODO, don't like this, should be part of input. oh well...

	u8 t,t2;
	demoplaying = true;
	int demopos = 0;
	u8 speed = 0;
	u8 cwait = 30;
	u8 coff = 0;
	t2 = 0;

	HideSprites(0,MAX_SPRITES);
	WaitVsync(1);

	level = demonum;


	//for(u8 i=0;i<5;i++)
	//	restore[i] = GetMapTile(5+i,0);//save the tiles under the overlaid text	

	while(demonum--){//seek to start position
		while(pgm_read_byte(&Demos[demopos++]) != 17);//terminating number, 17 = UP,DOWN,UP,DOWN a useless move so the demos wont have it	
	}
	LoadLevel(true,true,false,false,false,false);
	FadeIn(1,true);
	Render();//draw player once
	printrainbow(12,27,PSTR("5DEMO5"),24,0xC0,0,0);

	
	WaitVsync(60);

	while(true){
		
		t = pgm_read_byte(&Demos[demopos++]);
	  if(!opentargets || t == 17){//demo end
		  return DemoQuit();
	  }
	  
	  for(u8 i=0;i<4;i++){//each byte holds up to 4 moves
			if(opentargets){
			 t2 = (( t>>(i*2) ) & 3);//get 2 bits per move
			
			 if(t2==0)	  {t2=UP	|MOVE;}//convert 0,1,2,3 to UP,DOWN,LEFT,RIGHT
			 else if(t2==1){t2=DOWN |MOVE;}
			 else if(t2==2){t2=LEFT |MOVE;}
			 else			 {t2=RIGHT|MOVE;}
			}

		 pstate = t2;
		  for(u8 i=0;i<16;i++){//1 demo tick equals 1 full movement

				FillPad();
				if(StartDown()){//allow user to skip demo
					WaitVsync(1);
					return DemoQuit();
				}
			
			Logic();
	
			if(padstate & BTN_SR && !(oldpadstate & BTN_SR)){//fast forward
				if(speed == 0)
					speed = 1;
				else if(speed == 1)
					speed = 3;
					else
					speed = 7;

				Render();
			 }
			else if(padstate & BTN_SL && !(oldpadstate & BTN_SL)){//slow down
				if(speed == 1)
					speed = 0;
				else if(speed == 3)
					speed = 1;
					else if(speed == 7)
					speed = 3;

				Render();
			 }			  

				for(u8 j=0;j<speed;j++){
				Logic();
				i++;
			}

			spritecount = 0;
			Render();

				
			if(!cwait){//handle colored text
				  cwait = 30;
				  printrainbow(12,27,PSTR("5DEMO5"),24,0xC0,0,coff++);
				 if(coff > 3){coff = 0;}
			  }
			  else
				  cwait--;	
				}
		 }
	}//while true
}

void CalculateOptimum(u8 demonum){//scan through a demo and determine optimum moves/pushes(apparently solver program isn't perfect!)
	int demopos = 0;
	u8 t,t2;	
	level = demonum;

	demoplaying = true;//avoid saving progress or anything else

	while(demonum--){//seek to start position
		while(pgm_read_byte(&Demos[demopos++]) != 17);//terminating number, 17 = UP,DOWN,UP,DOWN a useless move so the demos wont have it	
	}

	LoadLevel(false,false,false,false,false,true);
	gamestate |= SCANNINGOPTIMUM;//gross hack to avoid triggering sounds etc.

	while(true){
		
		t = pgm_read_byte(&Demos[demopos++]);
	  if(!opentargets || t == 17)
			goto DemoScanEnd;//just wanted to have 1 ;)

	  for(u8 i=0;i<4;i++){//each byte holds up to 4 moves
			if(opentargets){
			 t2 = (( t>>(i*2) ) & 3);//get 2 bits per move
			
			 if(t2==0)	  {t2=UP	|MOVE;}//convert 0,1,2,3 to UP,DOWN,LEFT,RIGHT
			 else if(t2==1){t2=DOWN |MOVE;}
			 else if(t2==2){t2=LEFT |MOVE;}
			 else			 {t2=RIGHT|MOVE;}

			pstate = t2;
			poffset = 0;//setup move
			Logic();
			poffset = 16;//finish move
			Logic();
			}
		}//for
	}//while

DemoScanEnd://TODO FIX ME IF THERE IS "ENOUGH EXTRA SPACE" :D lol
	optimummoves = moves;
	optimumpushes = pushes;
	gamestate =0;//^= SCANNINGOPTIMUM;
	demoplaying = false;
}



void EngineInit(){
	ClearVram();
	demonum = 0;
	gamestate = 0;
	guistate = GMAINMENU;
	guijuststarted = true;
	tileset = 0;
	tracknum = 1;
	numlevels = NUMLEVELS;
	InitMusicPlayer(patches);
	SetSpritesTileTable(GameSprites);
	SetTileTable(GameTiles);
	SetMasterVolume(192);
	GameIntro();
}
	
void main(){
	EngineInit();

	while(true){
		Input();
		Gui();
		Logic();
		Render();
	}
}

