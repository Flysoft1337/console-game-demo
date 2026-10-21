#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>
using namespace std;

bool KeyPressed(int key){
    return GetAsyncKeyState(key) & 0x8000;
}

int main(){
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    if(!GetConsoleScreenBufferInfo(hConsole, &consoleInfo)){
        fputs("PYS requires an interactive Windows console.\n", stderr);
        return 1;
    }
    CONSOLE_CURSOR_INFO originalCursor;
    bool restoreCursor = GetConsoleCursorInfo(hConsole, &originalCursor) != FALSE;
    CONSOLE_CURSOR_INFO hiddenCursor = {1, FALSE};
    SetConsoleCursorInfo(hConsole, &hiddenCursor);

    const int MAP_W = 128;
    const int MAP_H = 32;
    const int RESOURCE_RATE = 6;
    bool resource[MAP_H][MAP_W] = {};
    int remainResource = 0;
    srand(static_cast<unsigned int>(time(nullptr)));
    for(int row = 0; row < MAP_H; row++){
        for(int column = 0; column < MAP_W; column++){
            if(rand() % 100 < RESOURCE_RATE){
                resource[row][column] = true;
                remainResource++;
            }
        }
    }

    //画面坐标相对可见窗口,不修改终端尺寸或窗口样式。
    SMALL_RECT viewport = {-1, -1, -1, -1};
    COORD bufferSize = {};
    CHAR_INFO blank = {};
    blank.Char.AsciiChar = ' ';
    blank.Attributes = consoleInfo.wAttributes;
    vector<CHAR_INFO> frame;
    vector<CHAR_INFO> displayed;
    bool needsPresent = false;
    bool forceRedraw = true;
    auto setCell = [&](int column, int row, char value){
        if(column >= 0 && column < bufferSize.X && row >= 0 && row < bufferSize.Y)
            frame[row * bufferSize.X + column].Char.AsciiChar = value;
    };
    auto writeText = [&](int column, int row, const string& text, int width){
        int count = min(width, static_cast<int>(text.size()));
        for(int offset = 0; offset < count; offset++)
            setCell(column + offset, row, text[offset]);
    };
    auto present = [&](){
        if(!needsPresent)
            return;
        SMALL_RECT dirty = {bufferSize.X, bufferSize.Y, -1, -1};
        if(forceRedraw){
            dirty = SMALL_RECT{0, 0, static_cast<SHORT>(bufferSize.X - 1),
                                   static_cast<SHORT>(bufferSize.Y - 1)};
        }
        else{
            //比较最终画面,避免把合成过程中的临时变化提交给终端。
            for(int row = 0; row < bufferSize.Y; row++){
                for(int column = 0; column < bufferSize.X; column++){
                    int index = row * bufferSize.X + column;
                    if(frame[index].Char.AsciiChar == displayed[index].Char.AsciiChar)
                        continue;
                    dirty.Left = static_cast<SHORT>(min<int>(dirty.Left, column));
                    dirty.Right = static_cast<SHORT>(max<int>(dirty.Right, column));
                    dirty.Top = static_cast<SHORT>(min<int>(dirty.Top, row));
                    dirty.Bottom = static_cast<SHORT>(max<int>(dirty.Bottom, row));
                }
            }
        }
        if(dirty.Left > dirty.Right || dirty.Top > dirty.Bottom){
            needsPresent = false;
            return;
        }
        COORD source = {dirty.Left, dirty.Top};
        SMALL_RECT target = {
            static_cast<SHORT>(viewport.Left + dirty.Left),
            static_cast<SHORT>(viewport.Top + dirty.Top),
            static_cast<SHORT>(viewport.Left + dirty.Right),
            static_cast<SHORT>(viewport.Top + dirty.Bottom)
        };
        SMALL_RECT written = target;
        if(WriteConsoleOutputA(hConsole, frame.data(), bufferSize, source, &written) &&
           written.Left == target.Left && written.Top == target.Top &&
           written.Right == target.Right && written.Bottom == target.Bottom){
            displayed = frame;
            needsPresent = false;
            forceRedraw = false;
        }
        else{
            //窗口可能在查询尺寸和提交之间变化,下一帧保留并重试。
            forceRedraw = true;
        }
    };

    int x = 10;
    int y = 10;
    int resourceCount = 0;
    int scene = 0;
    int exitCode = 0;
    while(true){
        if(KeyPressed('Q'))
            break;
        if(!GetConsoleScreenBufferInfo(hConsole, &consoleInfo)){
            exitCode = 1;
            break;
        }
        const SMALL_RECT current = consoleInfo.srWindow;
        bool resized = current.Left != viewport.Left || current.Top != viewport.Top ||
                       current.Right != viewport.Right || current.Bottom != viewport.Bottom;
        if(resized){
            viewport = current;
            bufferSize.X = viewport.Right - viewport.Left + 1;
            bufferSize.Y = viewport.Bottom - viewport.Top + 1;
            frame.assign(static_cast<size_t>(bufferSize.X) * bufferSize.Y, blank);
            displayed = frame;
            forceRedraw = true;
            needsPresent = true;
        }
        int width = bufferSize.X;
        int height = bufferSize.Y;
        bool canPlay = width >= 32 && height >= 12;
        int oldScene = scene;
        int oldX = x;
        int oldY = y;
        int oldResourceCount = resourceCount;
        int oldRemainResource = remainResource;
        if(KeyPressed(VK_RIGHT)) scene = 1;
        if(KeyPressed(VK_LEFT)) scene = 0;
        if(canPlay && scene == 0){
            if(remainResource <= 50){
                for(int row = 0; row < MAP_H; row++){
                    for(int column = 0; column < MAP_W; column++){
                        if(!resource[row][column] && rand() % 100 < RESOURCE_RATE){
                            resource[row][column] = true;
                            remainResource++;
                        }
                    }
                }
            }
            if(KeyPressed('W') && y > 0) y--;
            if(KeyPressed('S') && y < MAP_H - 1) y++;
            if(KeyPressed('A') && x > 0) x--;
            if(KeyPressed('D') && x < MAP_W - 1) x++;
            if(KeyPressed('E') && resource[y][x]){
                resource[y][x] = false;
                remainResource--;
                resourceCount++;
            }
        }
        bool changed = resized || scene != oldScene || x != oldX || y != oldY ||
                       resourceCount != oldResourceCount || remainResource != oldRemainResource;
        if(changed){
            fill(frame.begin(), frame.end(), blank);
            needsPresent = true;
            if(!canPlay){
                writeText(0, 0, "Resize terminal: minimum 32 x 12", width);
                writeText(0, 1, "Q Quit", width);
            }
            else if(scene == 0){
                bool sidebar = width >= 80;
                int viewW = min(MAP_W, width - (sidebar ? 31 : 0));
                int viewH = min(MAP_H, height - (sidebar ? 0 : 6));
                int cameraX = clamp(x - viewW / 2, 0, MAP_W - viewW);
                int cameraY = clamp(y - viewH / 2, 0, MAP_H - viewH);
                for(int row = 0; row < viewH; row++){
                    for(int column = 0; column < viewW; column++){
                        int mapX = cameraX + column;
                        int mapY = cameraY + row;
                        char value = mapX == x && mapY == y ? '@' :
                                     (resource[mapY][mapX] ? '#' : ' ');
                        setCell(column, row, value);
                    }
                }
                string position = "X:" + to_string(x) + " Y:" + to_string(y);
                if(sidebar){
                    for(int row = 0; row < height; row++)
                        setCell(viewW, row, '|');
                    int column = viewW + 1;
                    int infoWidth = width - column;
                    writeText(column, 0, "PYS", infoWidth);
                    writeText(column, 2, position, infoWidth);
                    writeText(column, 3, "Collected:" + to_string(resourceCount), infoWidth);
                    writeText(column, 4, "Remaining:" + to_string(remainResource), infoWidth);
                    writeText(column, 6, "Map:128 x 32", infoWidth);
                    writeText(column, 7, "View:" + to_string(viewW) + " x " + to_string(viewH), infoWidth);
                    writeText(column, 9, "WASD Move  E Collect", infoWidth);
                    writeText(column, 10, "Right Shop  Q Quit", infoWidth);
                }
                else{
                    for(int column = 0; column < width; column++)
                        setCell(column, viewH, '-');
                    writeText(0, viewH + 1, position, width);
                    writeText(0, viewH + 2, "Collected:" + to_string(resourceCount), width);
                    writeText(0, viewH + 3, "Remaining:" + to_string(remainResource), width);
                    writeText(0, viewH + 4, "WASD Move  E Collect", width);
                    writeText(0, viewH + 5, "Right Shop  Q Quit", width);
                }
            }
            else{
                int shopWidth = min(48, width - 2);
                int column = (width - shopWidth) / 2;
                int row = (height - 9) / 2;
                writeText(column, row, "========== SHOP ==========", shopWidth);
                writeText(column, row + 2, "Auto Robot: 100 resources", shopWidth);
                writeText(column, row + 3, "Purchase not implemented", shopWidth);
                writeText(column, row + 4, "Owned Bots: 0", shopWidth);
                writeText(column, row + 6, "Collected:" + to_string(resourceCount), shopWidth);
                writeText(column, row + 8, "Left Game   Q Quit", shopWidth);
            }
        }
        present();
        Sleep(30);
    }
    if(restoreCursor)
        SetConsoleCursorInfo(hConsole, &originalCursor);
    return exitCode;
}
