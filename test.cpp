#include <iostream>
#include <windows.h>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <string>
using namespace std;

bool KeyPressed(int key){
    return GetAsyncKeyState(key) & 0x8000;
}
int main(){
    system("cls");
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    cursorInfo.dwSize = 1;
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
    
    //定义范围
    const int ACTIVE_W = 128;
    const int ACTIVE_H = 32;
    const int INFO_STARTPOS = ACTIVE_W;
    const int INFO_W = 96;
    const int BUF_W = ACTIVE_W + INFO_W;
    const int BUF_H = ACTIVE_H;

    //定义物资
    bool resource[ACTIVE_H][ACTIVE_W] = {false};
    int remainResource = 0;
    srand((unsigned int)time(NULL));
    const int RESOURCE_RATE = 6;
    for(int y = 0; y < ACTIVE_H; y++){
        for(int x = 0; x < ACTIVE_W; x++){
            if(rand() % 100 < RESOURCE_RATE){
                resource[y][x] = true;
                remainResource++;
            }
        }
    }

    //缓冲区
    COORD bufferSize;
    bufferSize.X = BUF_W;
    bufferSize.Y = BUF_H;
    SetConsoleScreenBufferSize(hConsole, bufferSize);
    
    //可视化窗口
    SMALL_RECT windowRect;
    windowRect.Left = 0;
    windowRect.Top = 0;
    windowRect.Right = BUF_W - 1;
    windowRect.Bottom = BUF_H - 1;
    SetConsoleWindowInfo(hConsole, TRUE, &windowRect);
    
    //绘制资源
    auto drawResources = [&](){
        for(int y = 0; y < ACTIVE_H; y++){
            for(int x = 0; x < ACTIVE_W; x++){
                if(resource[y][x]){
                    COORD pos;
                    pos.X = x;
                    pos.Y = y;
                    SetConsoleCursorPosition(hConsole, pos);
                    cout<<"#";
                }
            }
        }
    };
    drawResources();

    auto clearScreen = [&](){
        DWORD written;
        FillConsoleOutputCharacterA(hConsole, ' ', BUF_W * BUF_H,
                                    COORD{0, 0}, &written);
    };
    auto writeInfo = [&](SHORT row, const string& text){
        SetConsoleCursorPosition(hConsole, COORD{INFO_STARTPOS, row});
        cout << left << setw(INFO_W) << text;
    };
    
    //禁止拖拽
    HWND hWdn = GetConsoleWindow();
    LONG style = GetWindowLongA(hWdn, GWL_STYLE);
    style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
    SetWindowLongA(hWdn, GWL_STYLE, style);
    
    //基础设置
    int x = 10;
    int y = 10;
    int oldX = x;
    int oldY = y;
    int resourceCount = 0;
    int scene = 0;

    //游戏循环
    while(true){
        if(KeyPressed('Q'))
            break;
        int oldScene = scene;
        if(KeyPressed(VK_RIGHT)){
            scene = 1;
        }
        if(KeyPressed(VK_LEFT)){
            scene = 0;
        }
        if(scene != oldScene){
            clearScreen();
            if(scene == 0)
                drawResources();
        }
        if(scene == 0){
            if(remainResource <= 50){
                for(int yy = 0; yy < ACTIVE_H; yy++){
                    for (int xx = 0; xx < ACTIVE_W; xx++){
                        if(!resource[yy][xx]){
                            if(rand() % 100 < RESOURCE_RATE){
                                resource[yy][xx] = true;
                                remainResource++;
                                COORD pos;
                                pos.X = xx;
                                pos.Y = yy;
                                SetConsoleCursorPosition(hConsole, pos);
                                cout << "#";
                            }
                        }
                    }
                }
            }
    
            //快捷键设置
            if(KeyPressed('W') && y > 0)
                y--; 
            if(KeyPressed('S') && y < ACTIVE_H - 1)
                y++;
            if(KeyPressed('A') && x > 0)
                x--;
            if(KeyPressed('D') && x < ACTIVE_W - 1)
                x++;
            if(KeyPressed('E')){
                if(resource[y][x]){
                    resource[y][x] = false;
                    remainResource--;
                    resourceCount = resourceCount + 1;
                    COORD clearPos;
                    clearPos.X = x;
                    clearPos.Y = y;
                    SetConsoleCursorPosition(hConsole, clearPos);
                    cout<<" ";
                }
            }

            COORD oldPos;
            oldPos.X = oldX;
            oldPos.Y = oldY;
            SetConsoleCursorPosition(hConsole, oldPos);
            if(resource[oldY][oldX])
                cout<<"#";
            else
                cout<<" ";

            COORD newPos;
            newPos.X = x;
            newPos.Y = y;
            SetConsoleCursorPosition(hConsole, newPos);
            cout<<"@";
            oldX = x;
            oldY = y;

            //坐标信息
            writeInfo(2, "X:" + to_string(x));
            writeInfo(3, "Y:" + to_string(y));
            //获得资源数
            writeInfo(4, "Resource count:" + to_string(resourceCount));
            //剩余资源数
            writeInfo(5, "Remain resource count:" + to_string(remainResource));
            //可游玩范围
            writeInfo(6, "The map area : " + to_string(ACTIVE_W) + " x " + to_string(ACTIVE_H));
            //信息栏范围
            writeInfo(7, "The info bar area : " + to_string(INFO_STARTPOS + 1) + " ~ " + to_string(BUF_W));
            //提示商店页面
            writeInfo(9, "Press 'rightarrow' for shop");
        }
        else if(scene == 1){
            COORD shopPos;
            shopPos.X = 10;
            //标题
            shopPos.Y = 5;
            SetConsoleCursorPosition(hConsole, shopPos);
            cout<<"==========<<SHOP>>==========";
            //自动机器人购买标识
            shopPos.Y = 7;
            SetConsoleCursorPosition(hConsole, shopPos);
            cout<<"Auto Robot: purchase 100 resource to buy 1";
            //你拥有的机器人总量
            shopPos.Y = 9;
            SetConsoleCursorPosition(hConsole, shopPos);
            cout<<"Owned Bots: 0";
            //你还剩多少资源
            shopPos.Y = 11;
            SetConsoleCursorPosition(hConsole, shopPos);
            cout<<"resource count: "<<resourceCount;
            //提示回到主页面
            shopPos.Y = 13;
            SetConsoleCursorPosition(hConsole, shopPos);
            cout<<"Press 'leftarrow' for gamescene";
        }
        Sleep(30);
    }
    return 0;
}
