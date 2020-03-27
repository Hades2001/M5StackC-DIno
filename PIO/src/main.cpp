#include <M5StickC.h>

extern uint8_t Dino[83844];
uint16_t Gray16[256];

TFT_eSprite GameSprite = TFT_eSprite(&M5.Lcd);

#define DINO_W 1233
#define DINO_H 68
void drawPixmap(uint16_t pos_x,
                uint16_t pos_y,
                uint16_t ins_x,
                uint16_t ins_y,
                uint16_t w,
                uint16_t h)
{
    uint8_t color_index;
    for (size_t y = 0; y < h; y++)
    {
        for (size_t x = 0; x < w; x++)
        {
            color_index = Dino[(ins_y + y) * DINO_W + (ins_x + x)];
            if (color_index != 0x00)
                GameSprite.drawPixel(pos_x + x, pos_y + y, Gray16[color_index]);
        }
    }
}

void drawRing(uint64_t index)
{
    uint8_t color_index;
    for (size_t y = 0; y < 12; y++)
    {
        for (size_t x = 0; x < 160; x++)
        {
            color_index = Dino[(54 + y) * DINO_W + (2 + ((x + index) % 1200))];
            if (color_index != 0x00)
                GameSprite.drawPixel(x, 68 + y, Gray16[color_index]);
        }
    }
}

// ground   [2:1202]    1200    [54:66]     12
//          [848:892]   44      [2:49]      47
// cloud    [86:132]    46      [2:15]      13
//          []
//          [431:482]   51      [2:52]      50
//          [382:431]   49      [2:52]      50
//          [332:382]   50      [2:52]      50

typedef struct Elves
{
    int16_t posx;
    int16_t posy;
    uint16_t w;
    uint16_t h;
    uint8_t *datapty = nullptr;
    bool    collisionflag;
    int8_t collisionoffset;
} Elves_t;

Elves_t *creatElves(uint16_t w,
                    uint16_t h,
                    uint16_t ins_x,
                    uint16_t ins_y)
{
    Elves_t *Elvesptr = (Elves_t *)malloc(sizeof(Elves_t));
    if (Elvesptr == nullptr)
        return nullptr;
    Elvesptr->datapty = (uint8_t *)malloc(sizeof(uint8_t) * w * h);
    Elvesptr->posx = 0;
    Elvesptr->posy = 0;
    Elvesptr->w = w;
    Elvesptr->h = h;
    Elvesptr->collisionoffset = 0;

    if (Elvesptr->datapty == nullptr)
    {
        free(Elvesptr);
        return nullptr;
    }
    for (size_t y = 0; y < h; y++)
    {
        for (size_t x = 0; x < w; x++)
        {
            Elvesptr->datapty[(y * w) + x] = Dino[(ins_y + y) * DINO_W + (ins_x + x)];
        }
    }
    return Elvesptr;
}

void deleteElves(Elves_t* Elvesptr)
{
    if( Elvesptr == nullptr ) return;
    if( Elvesptr->datapty == nullptr ) return;
    free( Elvesptr->datapty );
    free( Elvesptr );
}

#define DIS_H   80
#define DIS_W   160

void posElves(Elves_t* Elvesptr,int16_t posx, int16_t posy)
{
    uint8_t pix = 0;
    for (size_t y = 0; y < Elvesptr->h; y++)
    {
        for (size_t x = 0; x < Elvesptr->w; x++)
        {
            pix = Elvesptr->datapty[(y * Elvesptr->w) + x];
            if( (( posx + x ) < 0 )||(( posx + x ) > DIS_W )||\
                (( posy + y ) < 0 )||(( posy + y ) > DIS_H )||\
                (( pix == 0x00)))
                 continue;

            GameSprite.drawPixel(posx + x,posy + y,Gray16[pix]);
        }
    }
    Elvesptr->posx = posx;
    Elvesptr->posy = posy;
}

void posElves(Elves_t* Elvesptr)
{
    uint8_t pix = 0;
    for (size_t y = 0; y < Elvesptr->h; y++)
    {
        for (size_t x = 0; x < Elvesptr->w; x++)
        {
            pix = Elvesptr->datapty[(y * Elvesptr->w) + x];
            if( (( Elvesptr->posx + x ) < 0 )||(( Elvesptr->posx + x ) > DIS_W )||\
                (( Elvesptr->posy + y ) < 0 )||(( Elvesptr->posy + y ) > DIS_H )||\
                (( pix == 0x00)))
                 continue;

            GameSprite.drawPixel(Elvesptr->posx + x,Elvesptr->posy + y,Gray16[pix]);
        }
    }
}

Elves_t* cloud;
bool cloudFlag = false;

struct GameSys
{
    uint64_t systick;   
    uint8_t speed; 

    uint64_t keycount;

    bool cactusFlag;
    bool crowFlag;
    bool clondFlag;

    Elves_t* cloud;
    Elves_t* cactue;
    Elves_t* Dinoptr[6];

    float DinoHight_Start;
    float DinoHight;
    float   dinoV;
    uint8_t dinostate;
    uint16_t jumptime;
    uint8_t DinoIndex;

    uint16_t BK_color;
}sys={  0,0,0,
        false,false,false,
        nullptr,
        nullptr,
        {nullptr,nullptr,nullptr},
        0,0,0,0,0,0,
        WHITE};

#define DINO_RUN        0
#define DINO_JUMP_K     1
#define DINO_JUMP_N     2
#define DINO_OVER       3
#define DINO_WAIT       4
#define DINO_SHOW       5

#define DINO_G_PAM      0.3


const uint16_t cactuemap[11][4]={
    {228,2,17,35},
    {245,2,17,35},
    {262,2,17,35},
    {279,2,17,35},
    {296,2,17,35},
    {313,2,17,35},
    {332,2,25,50},
    {357,2,24,50},
    {382,2,25,50},
    {407,2,24,50},
    {431,2,51,50}
};

const uint16_t dinomap[6][4]={
    {848,2,44,47},
    {936,2,44,47},
    {980,2,44,47},
    
    {1112,20,59,29},
    {1171,20,59,29},

    {1070,2,44,47},
};

const uint16_t TextFontMap[20][4] = {
    {655,2,11,11},  //0
    {666,2, 9,11},  //1
    {675,2,10,11},  //2
    {685,2,10,11},  //3
    {695,2,10,11},  //4
    {705,2,10,11},  //5
    {715,2,10,11},  //6
    {725,2,10,11},  //7
    {735,2,10,11},  //8
    {745,2,10,11},  //9
    {755,2,11,11},  //H
    {766,2,11,11},  //I

    {655,15,11,11}, //G
    {679,15,11,11}, //A
    {703,15,11,11}, //M
    {727,15,11,11}, //E
    {763,15,11,11}, //O
    {787,15,11,11}, //V
    {811,15,11,11}, //E
    {835,15,11,11}, //R
};

const uint16_t IconMap[2][4]
{
    {2,2,36,32},
};

uint16_t Dinoaum[3] = {848, 936, 980};
uint64_t i = 0;

void drawText(uint16_t posx, uint16_t posy, uint8_t index)
{
    drawPixmap(posx,posy,
    TextFontMap[index][0],
    TextFontMap[index][1],
    TextFontMap[index][2],
    TextFontMap[index][3]);
}
void drawGameOver()
{
    drawText(10 + 0 ,20,12);
    drawText(10 + 16,20,13);
    drawText(10 + 32,20,14);
    drawText(10 + 48,20,15);

    drawText(10 + 80,20,16);
    drawText(10 + 96,20,17);
    drawText(10 + 112,20,18);
    drawText(10 + 128,20,19);

    drawPixmap(64,40,
    IconMap[0][0],
    IconMap[0][1],
    IconMap[0][2],
    IconMap[0][3]);

    posElves(sys.Dinoptr[5],5,31 - sys.DinoHight );
}

void drawPoint( uint32_t point)
{
    if( sys.dinostate == DINO_SHOW )
    {
         drawText(160 - 12, 2, 10);
    }
    else
    {
        drawText(160 - 60 ,2,point /10000% 10);
        drawText(160 - 48 ,2,point /1000 % 10);
        drawText(160 - 36 ,2,point /100 % 10);
        drawText(160 - 24 ,2,point /10 % 10);
        drawText(160 - 12 ,2,point % 10);
    }
}

void flushCloud()
{
    if( sys.clondFlag == false )
    {
        if( random(0,200) == 1)
        {
            sys.cloud = creatElves(46,13,86,2);
            sys.cloud->posx = 160;
            sys.cloud->posy = random(0,40);
            sys.clondFlag = true;
        }
    }
    else
    {
        if( sys.systick % 4 == 0 )
        {
            sys.cloud->posx --;
        }
        posElves(sys.cloud);

        if( sys.cloud->posx <= -sys.cloud->w)
        {
            deleteElves(sys.cloud);
            sys.clondFlag = false;
        }
    }
}

void flushcactus()
{
    if(( sys.cactusFlag == false ))
    {
        if ( sys.systick < 400 ) return;
        if( random(0,50) == 1)
        {
            uint8_t index = random(0,200) % 11;
            uint8_t number = random(0,210) % 3;

            if( index <= 5 )
            {
                if(( index + number ) > 5 )
                {
                    number = 5 - index;
                }
            }
            else if( index ==  10 )
            {
                number = 0;
            }
            else if(( index > 5 )&&( index < 10 ))// [6:9]
            {
                number = ( number > 1 ) ? 1 :number;
                if(( index + number ) >= 10 )
                {
                    number = 9 - index;
                }
            }
            
            if( number == 0 )
            {
                sys.cactue = creatElves(cactuemap[index][2],
                                        cactuemap[index][3],
                                        cactuemap[index][0],
                                        cactuemap[index][1]);
            }
            else
            {
                uint16_t pixw = 0;

                pixw = cactuemap[ index + number ][0] + cactuemap[ index + number ][2] - cactuemap[ index ][0];

                sys.cactue = creatElves(pixw,
                                        cactuemap[index][3],
                                        cactuemap[index][0],
                                        cactuemap[index][1]);
            }
            
            sys.cactue->posx = 160;
            sys.cactue->posy = 80-3-sys.cactue->h;
            sys.cactue->collisionoffset = -3;
            sys.cactusFlag = true;
        }
    }
    else
    {
        sys.cactue->posx -= sys.speed;
        posElves(sys.cactue);

        if( sys.cactue->posx <= -sys.cactue->w)
        {
            deleteElves(sys.cactue);
            sys.cactusFlag = false;
        }
    }
}

void drawDino()
{
    if( M5.BtnA.isPressed() && ( sys.dinostate == DINO_RUN ))
    {
        sys.dinostate = DINO_JUMP_K;
        sys.DinoHight = 0;
        sys.jumptime = 0;
    }
    else if( M5.BtnA.isPressed() && ( sys.dinostate == DINO_JUMP_K ))
    {
        sys.keycount++;
        if( sys.keycount > 20 )
        {
            sys.dinostate = DINO_JUMP_N;
            sys.keycount = 0;
        }
    }

    if( M5.BtnA.isReleased()&&(sys.dinostate == DINO_JUMP_K))
    {
        sys.dinostate = DINO_JUMP_N;
        sys.keycount = 0;
    }

    if( sys.dinostate == DINO_RUN )
    {
        if( !M5.BtnB.isPressed())
        {
            sys.DinoIndex = (sys.systick / 10) % 2 + 1;
        }
        else
        {
            sys.DinoIndex = (sys.systick / 10) % 2 + 3;
        }
    }
    else if( sys.dinostate == DINO_SHOW )
    {
        sys.DinoHight = 0;
        sys.DinoIndex = (sys.systick / 10) % 2 + 1;
    }
    else
    {
        if( sys.dinostate == DINO_JUMP_K )
        {
            sys.dinoV = 4;
            
            sys.DinoHight_Start = sys.dinoV * sys.keycount;
            sys.DinoHight = sys.DinoHight_Start;
            
            sys.jumptime = 0;
        }
        else
        {
            sys.jumptime++;
            sys.DinoHight = sys.DinoHight_Start + ( sys.dinoV * sys.jumptime ) - (( DINO_G_PAM * (sys.jumptime*sys.jumptime)) / 2 );
        }

        if(( sys.DinoHight <= 0 )&&( sys.dinostate == DINO_JUMP_N ))
        {
            sys.DinoHight = 0;
            sys.dinostate = DINO_RUN;
        }
        sys.DinoIndex = 0;
    }
    if( sys.DinoIndex >= 3 )
        posElves(sys.Dinoptr[sys.DinoIndex],5,45 - sys.DinoHight );
    else
        posElves(sys.Dinoptr[sys.DinoIndex],5,31 - sys.DinoHight );
}

bool checkElvesCollision(Elves_t *ElvesA,Elves_t* ElvesB)
{
    /*
    GameSprite.drawRect(ElvesA->posx - ElvesA->collisionoffset,
                        ElvesA->posy - ElvesA->collisionoffset,
                        ElvesA->w + ElvesA->collisionoffset * 2,
                        ElvesA->h + ElvesA->collisionoffset * 2,
                        BLUE);
    GameSprite.drawRect(ElvesB->posx - ElvesB->collisionoffset,
                        ElvesB->posy - ElvesB->collisionoffset,
                        ElvesB->w + ElvesB->collisionoffset * 2,
                        ElvesB->h + ElvesB->collisionoffset * 2,
                        RED);
    */
    if(((ElvesA->posx - ElvesA->collisionoffset) + (ElvesA->w + ElvesA->collisionoffset * 2)> (ElvesB->posx - ElvesB->collisionoffset)) &&\
       ((ElvesB->posx - ElvesB->collisionoffset) + (ElvesB->w + ElvesB->collisionoffset * 2)> (ElvesA->posx - ElvesA->collisionoffset)) &&\
       ((ElvesA->posy - ElvesA->collisionoffset) + (ElvesA->h + ElvesA->collisionoffset * 2)> (ElvesB->posy - ElvesB->collisionoffset)) &&\
       ((ElvesB->posy - ElvesB->collisionoffset) + (ElvesB->h + ElvesB->collisionoffset * 2)> (ElvesA->posy - ElvesA->collisionoffset)))
            return true;
    else
            return false;
        
}

void setup()
{
    M5.begin();
    M5.Lcd.setRotation(1);
    M5.Axp.ScreenBreath(10);
    GameSprite.createSprite(160, 80);
    GameSprite.fillRect(0, 0, 160, 80, WHITE);
    for (size_t i = 0; i < 256; i++)
    {
        Gray16[i] = GameSprite.color565(i, i, i);
    }

    for (size_t i = 0; i < 6; i++)
    {
        sys.Dinoptr[i] = creatElves(dinomap[i][2],
                                    dinomap[i][3],
                                    dinomap[i][0],
                                    dinomap[i][1]);
        sys.Dinoptr[i]->collisionoffset = -8;
    }
    M5.update();
    if( M5.BtnB.isPressed())
    {
        sys.dinostate = DINO_SHOW;
    }
    else
    {
        sys.dinostate = DINO_WAIT;
    }
    sys.systick = 0;
    sys.speed = 2;
    sys.BK_color = WHITE;

    drawDino();
    drawRing(sys.systick);

    GameSprite.pushSprite(0, 0);
}


void loop()
{
    if( sys.dinostate == DINO_WAIT )
    {
        if( M5.BtnA.wasPressed() || M5.BtnB.wasPressed() )
        {
            sys.dinostate = DINO_RUN;
        }
    }
    else if( sys.dinostate == DINO_OVER )
    {
        drawGameOver();
        if( M5.BtnA.wasPressed() || M5.BtnB.wasPressed() )
        {
            GameSprite.fillRect(0, 0, 160, 80, sys.BK_color);
            sys.systick = 0;
            sys.speed = 2;
            sys.dinostate = DINO_RUN;

            sys.cactusFlag = false;
            sys.clondFlag = false;
            drawRing(sys.systick);
            flushCloud();
            flushcactus();
            drawDino();

            GameSprite.pushSprite(0, 0);
        }
    }
    else if( sys.dinostate == DINO_SHOW )
    {
        GameSprite.fillRect(0, 0, 160, 80, sys.BK_color);
        drawRing(sys.systick);
        flushCloud();
        flushcactus();
        drawDino();
        sys.systick = sys.systick + sys.speed;

        if( M5.BtnA.wasPressed() || M5.BtnB.wasPressed() )
        {
            GameSprite.fillRect(0, 0, 160, 80, sys.BK_color);
            sys.systick = 0;
            sys.speed = 2;
            sys.dinostate = DINO_WAIT;

            sys.cactusFlag = false;
            sys.clondFlag = false;
            drawRing(sys.systick);
            flushCloud();
            flushcactus();
            drawDino();

            GameSprite.pushSprite(0, 0);
        }
    }
    else
    {
        GameSprite.fillRect(0, 0, 160, 80, sys.BK_color);
        drawRing(sys.systick);
        flushCloud();
        flushcactus();
        drawDino();
        sys.systick = sys.systick + sys.speed;

        if( sys.cactusFlag == true )
        {
            if( checkElvesCollision(sys.Dinoptr[sys.DinoIndex],sys.cactue)==true )
            {
                sys.dinostate = DINO_OVER; 
            }
        }
    }
    if((( sys.systick % 10000 ) >= 0 )&&(( sys.systick % 10000 ) < 4873))
    {
        sys.BK_color = WHITE;
    }
    else if((( sys.systick % 10000 ) >= 4873 )&&(( sys.systick % 10000 ) < 5000))
    {
        sys.BK_color = ( 5000- ( sys.systick % 10000 )) * 2;
        sys.BK_color = GameSprite.color565( sys.BK_color, sys.BK_color, sys.BK_color );
    }
    else if((( sys.systick % 10000 ) >= 5000 )&&(( sys.systick % 10000 ) < 9873))
    {
        sys.BK_color = BLACK;
    }
    else if((( sys.systick % 10000 ) >= 9873 )&&(( sys.systick % 10000 ) < 10000))
    {
        sys.BK_color = (( sys.systick % 10000 ) - 9745) * 2;
        sys.BK_color = GameSprite.color565( sys.BK_color, sys.BK_color, sys.BK_color );
    }
    if( M5.Axp.GetBtnPress())
    {
        ESP.restart();
    }

    drawPoint(sys.systick / 10);
    GameSprite.pushSprite(0, 0);
    M5.update();
}