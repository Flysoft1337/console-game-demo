#include <windows.h>
#include <array>
#include <cstdlib>
#include <ctime>
#include <string>
using namespace std;

bool KeyPressed(int key){
    return GetAsyncKeyState(key) & 0x8000;
}
int main(){
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
    
    //在内存中合成画面,只提交发生变化的矩形区域。
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    WORD attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    if(GetConsoleScreenBufferInfo(hConsole, &consoleInfo))
        attributes = consoleInfo.wAttributes;
    array<CHAR_INFO, BUF_W * BUF_H> frame{};
    for(auto& cell : frame){
        cell.Char.AsciiChar = ' ';
        cell.Attributes = attributes;
    }
    SMALL_RECT dirty = {0, 0, BUF_W - 1, BUF_H - 1};
    auto setCell = [&](int column, int row, char value){
        auto& cell = frame[row * BUF_W + column];
        if(cell.Char.AsciiChar == value)
            return;
        cell.Char.AsciiChar = value;
        if(column < dirty.Left) dirty.Left = column;
        if(column > dirty.Right) dirty.Right = column;
        if(row < dirty.Top) dirty.Top = row;
        if(row > dirty.Bottom) dirty.Bottom = row;
    };
    auto writeText = [&](int column, int row, const string& text, int width){
        for(int offset = 0; offset < width; offset++){
            char value = offset < static_cast<int>(text.size()) ? text[offset] : ' ';
            setCell(column + offset, row, value);
        }
    };
    auto present = [&](){
        if(dirty.Left > dirty.Right || dirty.Top > dirty.Bottom)
            return;
        //API 会修改目标矩形,使用副本以便失败时保留待绘区域。
        SMALL_RECT target = dirty;
        COORD source = {dirty.Left, dirty.Top};
        if(WriteConsoleOutputA(hConsole, frame.data(), bufferSize, source, &target))
            dirty = SMALL_RECT{BUF_W, BUF_H, -1, -1};
    };

    //绘制资源
    auto drawResources = [&](){
        for(int y = 0; y < ACTIVE_H; y++){
            for(int x = 0; x < ACTIVE_W; x++){
                if(resource[y][x])
                    setCell(x, y, '#');
            }
        }
    };

    auto clearScreen = [&](){
        for(int row = 0; row < BUF_H; row++){
            for(int column = 0; column < BUF_W; column++)
                setCell(column, row, ' ');
        }
    };
    auto writeInfo = [&](int row, const string& text){
        writeText(INFO_STARTPOS, row, text, INFO_W);
    };
    
    //禁止拖拽
    HWND hWdn = GetConsoleWindow();
    LONG style = GetWindowLongA(hWdn, GWL_STYLE);
    style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
    SetWindowLongA(hWdn, GWL_STYLE, style);
    
    //基础设置
    int x = 10;
    int y = 10;
    int resourceCount = 0;
    int scene = 0;
    bool firstFrame = true;

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
        bool sceneChanged = firstFrame || scene != oldScene;
        if(sceneChanged){
            clearScreen();
            if(scene == 0)
                drawResources();
        }
        if(scene == 0){
            int oldX = x;
            int oldY = y;
            int oldResourceCount = resourceCount;
            int oldRemainResource = remainResource;
            if(remainResource <= 50){
                for(int yy = 0; yy < ACTIVE_H; yy++){
                    for (int xx = 0; xx < ACTIVE_W; xx++){
                        if(!resource[yy][xx]){
                            if(rand() % 100 < RESOURCE_RATE){
                                resource[yy][xx] = true;
                                remainResource++;
                                setCell(xx, yy, '#');
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
                }
            }

            if(x != oldX || y != oldY)
                setCell(oldX, oldY, resource[oldY][oldX] ? '#' : ' ');
            setCell(x, y, '@');

            //坐标信息
            if(sceneChanged || x != oldX)
                writeInfo(2, "X:" + to_string(x));
            if(sceneChanged || y != oldY)
                writeInfo(3, "Y:" + to_string(y));
            //获得资源数
            if(sceneChanged || resourceCount != oldResourceCount)
                writeInfo(4, "Resource count:" + to_string(resourceCount));
            //剩余资源数
            if(sceneChanged || remainResource != oldRemainResource)
                writeInfo(5, "Remain resource count:" + to_string(remainResource));
            if(sceneChanged){
                writeInfo(6, "The map area : " + to_string(ACTIVE_W) + " x " + to_string(ACTIVE_H));
                writeInfo(7, "The info bar area : " + to_string(INFO_STARTPOS + 1) + " ~ " + to_string(BUF_W));
                writeInfo(9, "Press 'rightarrow' for shop");
            }
        }
        else if(scene == 1 && sceneChanged){
            auto writeShop = [&](int row, const string& text){
                writeText(10, row, text, static_cast<int>(text.size()));
            };
            writeShop(5, "==========<<SHOP>>==========");
            writeShop(7, "Auto Robot: purchase 100 resource to buy 1");
            writeShop(9, "Owned Bots: 0");
            writeShop(11, "resource count: " + to_string(resourceCount));
            writeShop(13, "Press 'leftarrow' for gamescene");
        }
        present();
        firstFrame = false;
        Sleep(30);
    }
    return 0;
}
