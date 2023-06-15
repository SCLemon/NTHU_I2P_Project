#include <stdio.h>
#include <stdlib.h>
#include <allegro5/allegro5.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_image.h>

// --- general ---
long frames;
int score = 0;
char score_string[3];
FILE *best_score_file = NULL;
int best_score = 0;
char best_score_string[5];
int best_score_was_updated = 0;
int isOnMenu = 1;
int isOnHowToPlay = 0;

// 檢查初始化操作是否成功
void must_init(bool test, const char *description)
{
    if (test) return;
    printf("couldn't initialize %s\n", description);
    exit(1);
}
// 獲取玩家的最高分數 (生成名為 bestScore.txt 的文件)
void get_best_score()
{
    best_score_file = fopen("bestScore.txt", "r");
    if (best_score_file != NULL)
    {
        // 文件中讀取最高分數的字串，並存儲在 best_score_string 變數中。這裡假設最高分數的字串長度不超過 4 個字符。
        fgets(best_score_string, 4, best_score_file);
        // 使用 atoi 函式將 best_score_string 轉換為整數，存儲在 best_score 變數中。
        best_score = atoi(best_score_string);
    }
    else
    {
        // 文件不存在 -> 則創建一個新的文件並將最高分數設置為 0。
        best_score_file = fopen("bestScore.txt", "w");
        fputc('0', best_score_file);
    }
    fclose(best_score_file);
}
// 判斷是否刷新紀錄
int is_best_score()
{
    if (score > best_score) return 1;
    return 0;
}
// 更新最高分數
void update_best_score()
{
    best_score_file = fopen("bestScore.txt", "w");
    // 將 score_string（包含新的最高分數）寫入 best_score_file 文件中。score_string 是一個字串，表示新的最高分數。
    fputs(score_string, best_score_file);
    fclose(best_score_file);
    // 用於表示最高分數已經被更新。
    best_score_was_updated = 1;
}

// --- display ---
#define BUFFER_W 144
#define BUFFER_H 256

#define DISP_SCALE 3 // 窗口的縮放比例
#define DISP_W (BUFFER_W * DISP_SCALE)
#define DISP_H (BUFFER_H * DISP_SCALE)

ALLEGRO_DISPLAY *disp;
ALLEGRO_BITMAP *buffer;

// 畫面初始化
void disp_init(){
    // 啟用音頻緩衝區
    al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 1, ALLEGRO_SUGGEST);
    al_set_new_display_option(ALLEGRO_SAMPLES, 8, ALLEGRO_SUGGEST);
    // 創建一個顯示窗口
    disp = al_create_display(DISP_W, DISP_H); // 窗口的寬度和高度
    must_init(disp, "display");
    // 創建一個 bitmap 緩衝區
    buffer = al_create_bitmap(BUFFER_W, BUFFER_H); // 緩衝區的寬度和高度
    must_init(buffer, "bitmap buffer");
}
// 銷毀並釋放資源
void disp_deinit(){
    al_destroy_bitmap(buffer);
    al_destroy_display(disp);
}
// 直接繪製在 bitmap 緩衝區（避免閃爍、提高繪製功能）
void disp_pre_draw(){
    al_set_target_bitmap(buffer);
}
// 用於完成繪製操作後的後處理步驟
void disp_post_draw()
{
    // 直接於顯示器上繪製
    al_set_target_backbuffer(disp);
    // 按照指定的大小和位置進行縮放繪製到顯示器上
    al_draw_scaled_bitmap(buffer, 0, 0, BUFFER_W, BUFFER_H, 0, 0, DISP_W, DISP_H, 0);
    // 將繪製的內容更新到顯示器上
    al_flip_display();
}

// --- keyboard ---

#define KEY_SEEN 1 // 按鍵按下
#define KEY_RELEASED 2 //按鍵釋放
unsigned char key[ALLEGRO_KEY_MAX]; // 儲存按鍵的狀態，每個元素對應一個特定的按鍵。
// 初始化
void keyboard_init(){
    memset(key, 0, sizeof(key)); // 全數設為零
}

// --- sprites ---
// 角色的寬度和長度
#define PLAYER_W 17
#define PLAYER_H 12
// 管道的寬度和長度
#define PIPE_W 26
#define PIPE_H 160

typedef struct SPRITES{
    ALLEGRO_BITMAP *player_sheet;
    ALLEGRO_BITMAP *player;
    ALLEGRO_BITMAP *background;
    ALLEGRO_BITMAP *floor;
    ALLEGRO_BITMAP *pipe_sheet;
    ALLEGRO_BITMAP *pipe[2];
    ALLEGRO_BITMAP *game_over;
    ALLEGRO_BITMAP *scoreboard;
    ALLEGRO_BITMAP *medal_sheet;
    ALLEGRO_BITMAP *medal[4];
    ALLEGRO_BITMAP *new_best;
    ALLEGRO_BITMAP *buttons;
    ALLEGRO_BITMAP *bt_ok;
    ALLEGRO_BITMAP *bt_menu;
    ALLEGRO_BITMAP *get_ready;
    ALLEGRO_BITMAP *how_to_play;
    ALLEGRO_BITMAP *game_title;
    ALLEGRO_BITMAP *play_button;
} SPRITES;

SPRITES sprites;
// 從一個原始圖像資源中提取指定位置和大小的子圖像。它創建並返回這個子圖像，並進行錯誤處理以確保子圖像的創建成功。
ALLEGRO_BITMAP *sprite_grab(ALLEGRO_BITMAP *sheet, int x, int y, int w, int h)
{
    ALLEGRO_BITMAP *sprite = al_create_sub_bitmap(sheet, x, y, w, h);
    must_init(sprite, "sprite grab");
    return sprite;
}
// 初始化遊戲中的圖像
void sprites_init()
{
    // 背景圖像
    sprites.background = al_load_bitmap("./assets/background.png");
    must_init(sprites.background, "background");
    // 地板圖像
    sprites.floor = al_load_bitmap("./assets/floor.png");
    must_init(sprites.floor, "floor");
    // 管道圖像資源 (包含多個管道的圖片)
    sprites.pipe_sheet = al_load_bitmap("./assets/pipe.png");
    must_init(sprites.pipe_sheet, "pipe");
    sprites.pipe[0] = sprite_grab(sprites.pipe_sheet, 0, 0, PIPE_W, PIPE_H);
    sprites.pipe[1] = sprite_grab(sprites.pipe_sheet, PIPE_W + 2, 0, PIPE_W, PIPE_H);
    // 計分板圖像
    sprites.scoreboard = al_load_bitmap("./assets/scoreboard.png");
    must_init(sprites.scoreboard, "scoreboard");
    // 新紀錄圖像
    sprites.new_best = al_load_bitmap("./assets/new.png");
    must_init(sprites.new_best, "new best score");
    // 按鈕圖像資源（包含多個按鈕的圖片）
    sprites.buttons = al_load_bitmap("./assets/buttons.png");
    must_init(sprites.buttons, "buttons");
    
    // 從圖像資源中提取子圖像，並將這些子圖像分配給相應的 ALLEGRO_BITMAP 指標成員。
    sprites.bt_menu = sprite_grab(sprites.buttons, 0, 0, 40, 14);
    sprites.bt_ok = sprite_grab(sprites.buttons, 0, 16, 40, 14);

    // 獎牌圖像資源（包含多個獎牌的圖片）
    sprites.medal_sheet = al_load_bitmap("./assets/medal.png");
    must_init(sprites.medal_sheet, "medal");
    sprites.medal[0] = sprite_grab(sprites.medal_sheet, 0, 72, 22, 22);
    sprites.medal[1] = sprite_grab(sprites.medal_sheet, 0, 48, 22, 22);
    sprites.medal[2] = sprite_grab(sprites.medal_sheet, 0, 24, 22, 22);
    sprites.medal[3] = sprite_grab(sprites.medal_sheet, 0, 0, 22, 22);

    // 遊戲標題圖像
    sprites.game_title = al_load_bitmap("./assets/flappybird.png");
    must_init(sprites.game_title, "title");
    // 播放按鈕圖像
    sprites.play_button = al_load_bitmap("./assets/playbutton.png");
    must_init(sprites.play_button, "play");
    // 遊戲教學圖像
    sprites.how_to_play = al_load_bitmap("./assets/howtoplay.png");
    must_init(sprites.how_to_play, "tutorial");
    // 遊戲開始提示圖像
    sprites.get_ready = al_load_bitmap("./assets/getready.png");
    must_init(sprites.get_ready, "ready");
    // 遊戲結束圖像
    sprites.game_over = al_load_bitmap("./assets/gameover.png");
    must_init(sprites.game_over, "gameover");
    // 玩家角色圖像資源（包含多個玩家角色的圖片）
    sprites.player_sheet = al_load_bitmap("./assets/player.png");
    must_init(sprites.player_sheet, "playersheet");
    sprites.player = sprite_grab(sprites.player_sheet, 0, 0, PLAYER_W, PLAYER_H);
}

// 釋放所有佔有的資源與記憶體
void sprites_denit(){
    al_destroy_bitmap(sprites.player);
    al_destroy_bitmap(sprites.background);
    al_destroy_bitmap(sprites.floor);
    al_destroy_bitmap(sprites.pipe_sheet);
    al_destroy_bitmap(sprites.pipe[0]);
    al_destroy_bitmap(sprites.pipe[1]);
    al_destroy_bitmap(sprites.player_sheet);
    al_destroy_bitmap(sprites.scoreboard);
    al_destroy_bitmap(sprites.new_best);
    al_destroy_bitmap(sprites.medal_sheet);
    al_destroy_bitmap(sprites.medal[0]);
    al_destroy_bitmap(sprites.medal[1]);
    al_destroy_bitmap(sprites.medal[2]);
    al_destroy_bitmap(sprites.medal[3]);
    al_destroy_bitmap(sprites.game_over);
    al_destroy_bitmap(sprites.buttons);
    al_destroy_bitmap(sprites.bt_menu);
    al_destroy_bitmap(sprites.bt_ok);
    al_destroy_bitmap(sprites.game_title);
    al_destroy_bitmap(sprites.play_button);
    al_destroy_bitmap(sprites.how_to_play);
    al_destroy_bitmap(sprites.get_ready);
}


// --- audio ---
ALLEGRO_SAMPLE *background_sound; //declare , loaded audio sample or sound file
ALLEGRO_SAMPLE *jump_sound;
ALLEGRO_SAMPLE *score_sound;
ALLEGRO_SAMPLE *death_sound;
ALLEGRO_SAMPLE *transition_sound;

void audio_play(ALLEGRO_SAMPLE *audio, ALLEGRO_PLAYMODE loop)
{
    al_play_sample(audio, 0.75, 0, 1, loop, NULL);}
                //audio sample that we want to play
                //0.75: represents the playback speed
                //0: played at original volume
                //1: full right panning
                //loop: determines whether the audio sample should loop or not. if we set it 0, the sample will play once and stop.
                 

void audio_init()
{
    al_install_audio(); //initializes the audio system in allegro. It should be called before any other audio function can be used
    al_init_acodec_addon(); //handle a wider range of audio formats
    al_reserve_samples(128); //128 sample instances, enough resources are allocated to handle multiple samples playing at the same time

    background_sound = al_load_sample("./sounds/background.wav");
    must_init(background_sound, "background sound"); //must_init to check whether the sound was loaded to the background sound

    jump_sound = al_load_sample("./sounds/jump.wav");
    must_init(jump_sound, "jump sound");

    score_sound = al_load_sample("./sounds/score.wav");
    must_init(score_sound, "score sound");

    death_sound = al_load_sample("./sounds/death.wav");
    must_init(death_sound, "death sound");

    transition_sound = al_load_sample("./sounds/transition.wav");
    must_init(transition_sound, "transition sound");

    audio_play(background_sound, ALLEGRO_PLAYMODE_LOOP); //always continue unless you stop it
}

void audio_denit()
{
    al_destroy_sample(background_sound); //destroy since it will make the RAM more heavily
    al_destroy_sample(jump_sound);
    al_destroy_sample(score_sound);
    al_destroy_sample(death_sound);
    al_destroy_sample(transition_sound);
}

// --- hud ---
void score_draw(ALLEGRO_FONT *font, int isAlive) // to check the player is alive or dead 
{
    sprintf(score_string, "%i", score); //%i : more flexible of %d 
    if (isAlive)                        //sprintf is used to store the value of the score in to the score_string
    {
        al_draw_text(font, al_map_rgb(255, 255, 255), (BUFFER_W / 2) - 2, 10, 0, score_string);
    } //al_draw_text is for the text on the screen
        //font  is the font from allegro_font* library
        //al_map_rgb is used to  create a white color based on RGB color values
        //(buffer_w/2)-2: specifies the text horizontally (x coordinate)
        //buffer_w /2 represents half of the width of the screen and subtract 2, it shifts the position slightly to the left
        //score_string the value of the integer that we store in the sprintf
        // 10: y coordinate
        // 0 : additional flags
    else
    {
        al_draw_text(font, al_map_rgb(251, 120, 88), BUFFER_W - 40, BUFFER_H - 153, 0, score_string);
        //251 : red , 120 : green, 88: blue
        //buffer_w shift the position 40 pixels to the left
        //buffer_h shift the position 153 pixels from the bottom
        //0: additional flags
        ////score_string the value of the integer that we store in the sprintf
    }
}

void scoreboard_draw(ALLEGRO_FONT *font) //draw scoreboard
{   //al_draw_bitmap draw the bitmap onto the display //draw the first one onto the display  //buffer represent the width
    al_draw_bitmap(sprites.game_over, BUFFER_W / 9 + 8, BUFFER_W / 3 - 5, 0); 
    al_draw_bitmap(sprites.scoreboard, BUFFER_W / 9, BUFFER_H / 3, 0); //draw  score board
    sprintf(best_score_string, "%i", best_score);  //store the best_score into the best_score_String
    al_draw_text(font, al_map_rgb(251, 120, 88), BUFFER_W - 40, BUFFER_H - 133, 0, best_score_string);
    //draw text onto the display  //almaprgb for the color // buffer_w and buffer_h is x and y coordinates //0 : no special flags
    //best_score_String will be drawn 
    al_draw_bitmap(sprites.bt_ok, 27, 150, 0); //draw bt_ok
    al_draw_bitmap(sprites.bt_menu, 77, 150, 0);  //draw bt_menu
    if (best_score_was_updated)
    {
        al_draw_bitmap(sprites.new_best, 82, BUFFER_H - 142, 0); //draw best score
    }

    //draw the gold, silver or bronze medal according to the player's socre
    if (score < 10)
    {
        al_draw_bitmap(sprites.medal[0], (BUFFER_W / 9) + 13, (BUFFER_H / 3) + 21, 0);
    }
    if (score >= 10 && score < 20)
    {
        al_draw_bitmap(sprites.medal[1], (BUFFER_W / 9) + 13, (BUFFER_H / 3) + 21, 0);
    }
    if (score >= 20 && score < 30)
    {
        al_draw_bitmap(sprites.medal[2], (BUFFER_W / 9) + 13, (BUFFER_H / 3) + 21, 0);
    }
    if (score >= 30)
    {
        al_draw_bitmap(sprites.medal[3], (BUFFER_W / 9) + 13, (BUFFER_H / 3) + 21, 0);
    }
}

void menu_draw(ALLEGRO_FONT *font){ //drawing elements on a menu screen
    al_draw_bitmap(sprites.game_title, BUFFER_W / 5, BUFFER_H / 10, 0); //draw the game title
    al_draw_bitmap(sprites.play_button, BUFFER_W / 3.2, BUFFER_H / 3, 0); //draw the play buttom
    al_draw_bitmap(sprites.player, BUFFER_W - 30, 50, 0); //draw the player
}

void tutorial_draw(){ //draw tutorial
    al_draw_bitmap(sprites.get_ready, BUFFER_W / 9 + 8, BUFFER_W / 3 - 5, 0);  //get ready
    al_draw_bitmap(sprites.how_to_play, BUFFER_W / 3.5, BUFFER_H / 2, 0);   //how to play
}

// --- player ---

typedef struct PLAYER{
    float x, y, gravity;
    int isAlive;

} PLAYER;

PLAYER player;

void player_init(){
    player.x = 20; //the player initial's horizontal position
    player.y = 100; //the player initial's vertical position
    player.gravity = 0; //the player doesnt have any initial vertical acceleration due to gravity 
    player.isAlive = 1; //indicates the player initially alive or active 
}

void player_draw(){
    al_draw_bitmap(sprites.player, player.x, player.y, 0); //draw player
}

// --- obtacles ---

#define NPIPES_MAX 99999
#define PIPE_SPACE_BETWEEN 95

typedef struct PIPE{ //定義pipe位置和分數
    float x, y;
    int isScored;
} PIPE;

PIPE pipes[NPIPES_MAX];

int isPlayerColliding(float player_x, float player_y, float pipe_x, float pipe_y){ //此函數檢查玩家是否與pipe產生碰撞
    if (player_x + PLAYER_W >= pipe_x && player_x <= pipe_x + PIPE_W){ //x,y其中一座標座標重疊則產生碰撞
        if (player_y >= pipe_y && player_y <= pipe_y + PIPE_H){
            return 1;
        }
        if (player_y + PLAYER_H >= pipe_y + PIPE_H + PIPE_SPACE_BETWEEN && player_y + PLAYER_H <= pipe_y + PIPE_H + PIPE_H + PIPE_SPACE_BETWEEN){
            return 1;
        }
    }
    if (player_y <= 0 || player_y >= BUFFER_H){//超出畫布範圍則產生碰撞
        return 1;
    }
    return 0;
}

void pipe_init(){
    srand(time(NULL)); //初始化隨機數生成器。它將 time(NULL) 作為種子，以確保每次運行時生成的隨機數序列都是不同的。
    pipes[0].x = BUFFER_W;//水管的初始 x 座標在畫布的寬度邊界上
    pipes[0].y = (rand() % 100) - 100;//y座標將 pipes[0].y 設定為 (rand() % 100) - 100，這將生成一個介於 -100 和 -1 之間的隨機數。

    pipes[0].isScored = 0;//水管尚未被計分
    for (int i = 1; i < NPIPES_MAX; i++)
    {
        pipes[i].x = pipes[i - 1].x + 80;//設定水管間的水平間距
        pipes[i].y = (rand() % 100) - 100;//設定水管間的垂直間距

        pipes[i].isScored = 0;
    }
}

void pipe_update(){//更新水管狀態
    for (int i = 0; i < NPIPES_MAX; i++)
    {
        pipes[i].x = pipes[i].x - 0.7;

        if (isPlayerColliding(player.x, player.y, pipes[i].x, pipes[i].y)){//若玩家與水管發生碰撞，則玩家死亡
            player.isAlive = 0;
        }

        ////檢查水管是否已經通過了玩家並且未計分。
        if (pipes[i].x + PIPE_W < player.x && !pipes[i].isScored && player.isAlive){//若水管的右邊界小於玩家的左邊界，且該水管尚未被計分，並且玩家仍然存活，則將分數 score 增加 1
            score++;
            pipes[i].isScored = 1;
            audio_play(score_sound, ALLEGRO_PLAYMODE_ONCE);//播放音效
        }

    }
}

void pipe_draw(){ //此函式使用迴圈來繪製每個水管
    for (int i = 0; i < NPIPES_MAX; i++){//檢查水管是否在畫布範圍內
        if (pipes[i].x + PIPE_W > 0 && pipes[i].x < BUFFER_W){
            al_draw_bitmap(sprites.pipe[0], pipes[i].x, pipes[i].y, 0);
            al_draw_bitmap(sprites.pipe[1], pipes[i].x, pipes[i].y + PIPE_H + PIPE_SPACE_BETWEEN, 0);
        }
    }
}
//--- main ---
int main()
{
    must_init(al_init(), "allegro");
    must_init(al_install_keyboard(), "keyboard");
    must_init(al_install_mouse(), "mouse");
    //使用 al_create_timer 函式創建了一個計時器，速率為每秒觸發 1/ 60次，它將計時器的指標賦值給 timer 變數。接下來，調用 must_init 函式，將 timer 和描述性的字串 "timer" 作為參數傳遞。
    ALLEGRO_TIMER *timer = al_create_timer(1.0 / 60.0);
    must_init(timer, "timer");

    //使用 al_create_event_queue 函式創建了一個事件佇列。然後，它將事件佇列的指標賦值給 queue 變數。接下來，調用 must_init 函式，將 queue 和描述性的字串 "queue" 作為參數傳遞。
    ALLEGRO_EVENT_QUEUE *queue = al_create_event_queue();
    must_init(queue, "queue");

    al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 1, ALLEGRO_SUGGEST);
    al_set_new_display_option(ALLEGRO_SAMPLES, 8, ALLEGRO_SUGGEST);
    al_set_new_bitmap_flags(ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR);

    disp_init();

    must_init(al_init_image_addon(), "image");//初始化 Allegro 的圖像處理相關模組，確保可以載入圖像文件。
    sprites_init();//初始化遊戲中的精靈圖像??。

    ALLEGRO_FONT *font = al_create_builtin_font();//創建 Allegro 的內建字型
    must_init(font, "font");//調用 must_init 函式，將字型的指標和描述性的字串 "font" 作為參數傳遞，確保字型的創建成功。

    must_init(al_init_primitives_addon(), "primitives");//初始化 Allegro 的基本圖元繪製相關模組，確保可以繪製基本的圖元形狀，如線段、矩形、圓等。

    al_register_event_source(queue, al_get_keyboard_event_source());//將鍵盤、顯示事件、計時器和滑鼠的事件來源導入到事件佇列中，以便在遊戲迴圈中接收和處理這些事件。
    al_register_event_source(queue, al_get_display_event_source(disp));
    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_register_event_source(queue, al_get_mouse_event_source());

    keyboard_init();
    player_init();
    pipe_init();
    audio_init();

    bool done = false;
    bool redraw = true;
    ALLEGRO_EVENT event;

    al_start_timer(timer);
    while (1)
    {
        al_wait_for_event(queue, &event);//使用 al_wait_for_event 函式來等待事件的發生。函式會將事件從queue取出，並將其儲存在 event 變數中。

        switch (event.type)
        {
        case ALLEGRO_EVENT_TIMER:
            if (isOnMenu)//在選單畫面 (isOnMenu)，檢查是否按下 A 鍵 (ALLEGRO_KEY_A)。
            {
                if (key[ALLEGRO_KEY_A])//如果是，將 isOnMenu 設為 0，並將 isOnHowToPlay 設為 1。
                {
                    isOnMenu = 0;
                    isOnHowToPlay = 1;
                }
            }
            else//否則，如果目前在遊戲說明畫面 (isOnHowToPlay)，檢查是否按下空白鍵 (ALLEGRO_KEY_SPACE)。
            {
                if (isOnHowToPlay)//如果是，將 isOnHowToPlay 設為 0。
                {
                    if (key[ALLEGRO_KEY_SPACE])
                    {
                        isOnHowToPlay = 0;
                    }
                }
                else//更新pipe、玩家的重力、位置等。如果玩家存活，根據玩家是否按下空白鍵來調整玩家的位置。
                    pipe_update();
                    if (player.isAlive)
                    {
                        player.gravity = player.gravity + 0.3;
                        if (player.y < 461 && !key[ALLEGRO_KEY_SPACE])
                        {
                            player.y = player.y + player.gravity;
                        }
                        if (key[ALLEGRO_KEY_SPACE])
                        {
                            player.gravity = 0;
                        }
                        player.y = player.y - 2.4;
                    }
                    else //如果玩家死亡，檢查是否按下ESC，並根據情況重設遊戲相關變數。
                {
                    {
                        if (key[ALLEGRO_KEY_ESCAPE])
                        {
                            isOnHowToPlay = 1;
                            player_init();
                            pipe_init();
                            score = 0;
                            best_score_was_updated = 0;
                        }
                    }
                }
            }

            for (int i = 0; i < ALLEGRO_KEY_MAX; i++)//將按鍵狀態 (key) 中的 KEY_SEEN 清除，設定 redraw 為 true，表示需要重新繪製畫面。
                key[i] &= KEY_SEEN;

            redraw = true;
            break;

        case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:  // 當滑鼠按鈕按下事件發生時的處理邏輯
            if (isOnMenu) //檢查滑鼠按下的位置是否在選單畫面的特定區域內。
            {
                if (event.mouse.x >= DISP_W / 3.2 && event.mouse.x <= (DISP_W / 3.2) + 52 * 3)//滑鼠按下的是選單中的特定按鈕。
                {
                    if (event.mouse.y >= DISP_H / 3 && event.mouse.y <= (DISP_H / 3) + 29 * 3)
                    {
                        isOnMenu = 0;//將 isOnMenu 設為 0，表示選單畫面結束，並將 isOnHowToPlay 設為 1，進入遊戲說明畫面。
                        isOnHowToPlay = 1;
                    }
                }
            }
            else
            {
                if (!player.isAlive)//玩家死亡
                {
                    if (event.mouse.y >= 150 * DISP_SCALE && event.mouse.y <= 164 * DISP_SCALE)//檢查滑鼠按下的位置是否在遊戲結束時顯示的按鈕區域內。
                    {
                        ////如果滑鼠的 x 座標在 27 * DISP_SCALE 到 67 * DISP_SCALE 的範圍內，表示滑鼠按下的是重新開始遊戲的按鈕。
                        {
                        if (event.mouse.x >= 27 * DISP_SCALE && event.mouse.x <= 67 * DISP_SCALE)
                            isOnHowToPlay = 1;//將 isOnHowToPlay 設為 1，重置遊戲狀態
                            player_init();//重新初始化玩家、管道 ，重置分數 ，並將 best_score_was_updated 設為 0
                            pipe_init();
                            score = 0;
                            best_score_was_updated = 0;
                        }
                        if (event.mouse.x >= 77 * DISP_SCALE && event.mouse.x <= 117 * DISP_SCALE)//滑鼠按下的是返回選單的按鈕。
                        {
                            isOnMenu = 1;//將 isOnMenu 設為 1，返回選單畫面，同樣需要重置遊戲狀態。
                            player_init();
                            pipe_init();
                            score = 0;
                            best_score_was_updated = 0;
                        }
                    }
                }
            }
            break;

        case ALLEGRO_EVENT_KEY_DOWN:// 當鍵盤按下事件發生時的處理邏輯
            key[event.keyboard.keycode] = KEY_SEEN | KEY_RELEASED;
            break;
        case ALLEGRO_EVENT_KEY_UP:// 當鍵盤放開事件發生時的處理邏輯
            key[event.keyboard.keycode] &= KEY_RELEASED;
            break;

        case ALLEGRO_EVENT_DISPLAY_CLOSE:// 當顯示視窗關閉事件發生時的處理邏輯
            done = true;
            break;
        }

        if (done)
            break;

        if (redraw && al_is_event_queue_empty(queue))//檢查 redraw 變數和事件佇列是否為空。只有在 redraw 為真且事件佇列為空時，才需要重新繪製畫面。
        {
            disp_pre_draw();// 進行繪製前的準備工作。
            al_clear_to_color(al_map_rgb(0, 0, 0));// 函式將畫布清空為黑色背景。

            al_draw_bitmap(sprites.background, 0, 0, 0);//繪製背景圖像 (sprites.background)。

            if (isOnMenu)
            {
                menu_draw(font);
                al_draw_bitmap(sprites.floor, 0, BUFFER_H - 56, 0);
            }
            else
            {
                if (isOnHowToPlay)
                {
                    tutorial_draw();
                }

                pipe_draw();//繪製pipe。
                if (player.isAlive)
                {
                    player_draw();
                }
                else//玩家已死亡
                {
                    get_best_score();//如果是新的最高分數，則呼叫 update_best_score() 函式進行更新，
                    if (is_best_score())
                    {
                        update_best_score();
                        get_best_score();//再次呼叫 get_best_score() 函式獲取最新的最高分數。
                    }
                    scoreboard_draw(font);
                }
                score_draw(font, player.isAlive);//繪製分數。
            }
            disp_post_draw();//進行繪製後的清理工作。
            redraw = false;//完成重新繪製畫面的操作。
        }
    }

    sprites_denit();
    disp_deinit();
    al_destroy_font(font);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);
    audio_denit();

    return 0;
}
