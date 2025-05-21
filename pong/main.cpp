//linker::system::subsystem  - Windows(/ SUBSYSTEM:WINDOWS)
//configuration::advanced::character set - not set
//linker::input::additional dependensies Msimg32.lib; Winmm.lib

#include "windows.h"
#include <cmath>

// ������ ������ ����  
typedef struct {
	float width, height, x, y, rad, dx, dy, speed;
	HBITMAP hBitmap;//����� � ������� ������ 
	bool status;
} sprite;

sprite racket;//������� ������
sprite enemy;//������� ����������
sprite ball;//�����
sprite Trace;

const int brickRow = 20;
const int brickColumn = 5;

sprite brickArray[brickRow][brickColumn];

struct {
	int score, balls;//���������� ��������� ����� � ���������� "������"
	bool action = false;//��������� - �������� (����� ������ ������ ������) ��� ����
} game;

struct {
	HWND hWnd;//����� ����
	HDC device_context, context;// ��� ��������� ���������� (��� �����������)
	int width, height;//���� �������� ������� ���� ������� ������� ���������
} window;

struct {
	int x;
	int y;
} coordinate;

HBITMAP hBack;// ����� ��� �������� �����������

//c����� ����

void InitGame()
{
	//� ���� ������ ��������� ������� � ������� ������� gdi
	//���� ������������� - ����� ������ ������ ����� � .exe 
	//��������� ������ LoadImageA ��������� � ������� ��������, ��������� �������� ����� ������������� � ������� ���� �������
	ball.hBitmap = (HBITMAP)LoadImageA(NULL, "ball.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	racket.hBitmap = (HBITMAP)LoadImageA(NULL, "racket.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	enemy.hBitmap = (HBITMAP)LoadImageA(NULL, "racket_enemy.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	hBack = (HBITMAP)LoadImageA(NULL, "back.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	//------------------------------------------------------

	racket.width = 300;
	racket.height = 50;
	racket.speed = 30;//�������� ����������� �������
	racket.x = window.width / 2.;//������� ���������� ����
	racket.y = window.height - racket.height;//���� ���� ���� ������ - �� ������ �������

	ball.dy = (rand() % 65 + 35) / 100.;//��������� ������ ������ ������
	ball.dx = -(1 - ball.dy);//��������� ������ ������ ������
	ball.speed = 11;
	ball.rad = 20;
	ball.x = racket.x;//x ���������� ������ - �� ������� �������
	ball.y = racket.y - ball.rad;//����� ����� ������ �������
	Trace = ball;

	for (int i = 0; i < brickColumn; i++)
	{
		for (int j = 0; j < brickRow; j++)
		{
			brickArray[j][i].width = window.width / (float)brickRow;
			brickArray[j][i].height = window.height / 3 / (float)brickColumn;
			brickArray[j][i].x = brickArray[j][i].width * j;
			brickArray[j][i].y = brickArray[j][i].height * i + (window.height / 3);
			brickArray[j][i].hBitmap = enemy.hBitmap;
			brickArray[j][i].status = true;
		}
	}

	game.score = 0;
	game.balls = 9;


}

void ProcessSound(const char* name)//������������ ���������� � ������� .wav, ���� ������ ������ � ��� �� ����� ��� � ���������
{
	PlaySound(TEXT(name), NULL, SND_FILENAME | SND_ASYNC);//���������� name �������� ��� �����. ���� ASYNC ��������� ����������� ���� ���������� � ����������� ���������
}

void ShowScore()
{
	//�������� �������� � �������
	SetTextColor(window.context, RGB(160, 160, 160));
	SetBkColor(window.context, RGB(0, 0, 0));
	SetBkMode(window.context, TRANSPARENT);
	auto hFont = CreateFont(70, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 2, 0, "CALIBRI");
	auto hTmp = (HFONT)SelectObject(window.context, hFont);

	char txt[32];//����� ��� ������
	_itoa_s(game.score, txt, 10);//�������������� �������� ���������� � �����. ����� �������� � ���������� txt
	TextOutA(window.context, 10, 10, "Score", 5);
	TextOutA(window.context, 200, 10, (LPCSTR)txt, strlen(txt));

	_itoa_s(game.balls, txt, 10);
	TextOutA(window.context, 10, 100, "Balls", 5);
	TextOutA(window.context, 200, 100, (LPCSTR)txt, strlen(txt));
}

void ProcessInput()
{
	if (GetAsyncKeyState(VK_LEFT)) racket.x -= racket.speed;
	if (GetAsyncKeyState(VK_RIGHT)) racket.x += racket.speed;

	if (!game.action && GetAsyncKeyState(VK_SPACE))
	{
		game.action = true;
		//ProcessSound("bounce.wav");
	}
}

void ShowBitmap(HDC hDC, int x, int y, int x1, int y1, HBITMAP hBitmapBall, bool alpha = false)
{
	HBITMAP hbm, hOldbm;
	HDC hMemDC;
	BITMAP bm;

	hMemDC = CreateCompatibleDC(hDC); // ������� �������� ������, ����������� � ���������� �����������
	hOldbm = (HBITMAP)SelectObject(hMemDC, hBitmapBall);// �������� ����������� bitmap � �������� ������

	if (hOldbm) // ���� �� ���� ������, ���������� ������
	{
		GetObject(hBitmapBall, sizeof(BITMAP), (LPSTR)&bm); // ���������� ������� �����������

		if (alpha)
		{
			TransparentBlt(window.context, x, y, x1, y1, hMemDC, 0, 0, x1, y1, RGB(0, 0, 0));//��� ������� ������� ����� ����� ��������������� ��� ����������
		}
		else
		{
			StretchBlt(hDC, x, y, x1, y1, hMemDC, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY); // ������ ����������� bitmap
		}

		SelectObject(hMemDC, hOldbm);// ��������������� �������� ������
	}

	DeleteDC(hMemDC); // ������� �������� ������
}

void ShowBricks()
{
	for (int i = 0; i < brickColumn; i++)
	{
		for (int j = 0; j < brickRow; j++)
		{
			if (brickArray[j][i].status)
			{
				ShowBitmap(window.context, brickArray[j][i].x, brickArray[j][i].y, brickArray[j][i].width, brickArray[j][i].height, brickArray[j][i].hBitmap);//������� ���������
			}
		}
	}
}

float DegToRad(float deg)
{
	return (deg * 3.14 / 180);
}

void CheckBlockTrace(int x, int y, float dx, float dy)
{
	for (int i = 0; i < brickColumn; i++)
	{
		for (int j = 0; j < brickRow; j++)
		{
			int borderX = brickArray[j][i].width + brickArray[j][i].x;
			int borderY = brickArray[j][i].height + brickArray[j][i].y;
			if (x >= brickArray[j][i].x && x <= borderX && y >= brickArray[j][i].y && y <= borderY && brickArray[j][i].status)
			{
				int minX = min(x - brickArray[j][i].x, borderX - x);
				int minY = min(y - brickArray[j][i].y, borderY - y);

				Trace.x = x;
				Trace.y = y;

				if (minY < minX)
				{
					Trace.dy *= -1;
					return;
				}
				else {
					Trace.dx *= -1;
					return;
				}
			}
		}
	}
}

void CheckWallsTrace(int x, int y, float dx, float dy)
{
	if (x < ball.rad || x > window.width - ball.rad)
	{
		Trace.dx *= -1;
		Trace.x = x;
		Trace.y = y;
	}
}

void CheckRoofTrace(int x, int y, float dx, float dy)
{
	if (y < ball.rad)
	{
		Trace.dy *= -1;
		Trace.x = x;
		Trace.y = y;
	}
}

void CheckFloorTrace(int x, int y, float dx, float dy)
{
	if (y > window.height - ball.rad - racket.height)//����� ������� ����� ������� - ����������� �������
	{
		if (x >= x - racket.width / 2. - ball.rad && x <= racket.x + racket.width / 2. + ball.rad)//����� �����, � �� �� � ������ ��������� ������
		{
			Trace.dy *= -1;//������
			Trace.x = x;
			Trace.y = y;
		}
	}
}

int** ShowTrace()
{
	int** trace{ new int* [(ball.speed) + 1000] {} };
	for (unsigned i{}; i < (ball.speed) + 1000; i++)
	{
		trace[i] = new int[181] {};
	}
	/*for (unsigned i{}; i < (ball.speed) + 1; i++)
	{
		for (unsigned j{}; j < 181; j++)
		{
			trace[i][j] = new int[2] {};
		}
	}*/


	for (int i = 1; i < (ball.speed) + 1000; i++)
	{
		trace[i][0] = Trace.x;
		trace[i][1] = Trace.y;
	}
	if (game.action)
	{
		for (int i = 1; i < (ball.speed) + 1000; i += 1)
		{
			float x = Trace.dx * i;
			float y = Trace.dy * i;
			float a = atan2(y, x);
			float deg = DegToRad(90) + a;
			//for (int j = 0; j < 181; j++)
			//{
				//trace[i][j][0] = Trace.x + (x + ball.rad * cos(deg - DegToRad(j)));
				//trace[i][j][1] = Trace.y + (y + ball.rad * sin(deg - DegToRad(j)));
			trace[i][0] = Trace.x + x;
			trace[i][1] = Trace.y + y;
			SetPixel(window.context, trace[i][0], trace[i][1], RGB(0, 255, 0));
			CheckBlockTrace(trace[i][0], trace[i][1], x, y);
			CheckWallsTrace(trace[i][0], trace[i][1], x, y);
			CheckRoofTrace(trace[i][0], trace[i][1], x, y);
			CheckFloorTrace(trace[i][0], trace[i][1], x, y);

			//}            
		}
	}
	return trace;
}

void ShowRacketAndBall()
{
	ShowBitmap(window.context, 0, 0, window.width, window.height, hBack);//������ ���
	ShowBitmap(window.context, racket.x - racket.width / 2., racket.y, racket.width, racket.height, racket.hBitmap);// ������� ������  
	ShowBricks();
	ShowBitmap(window.context, ball.x - ball.rad, ball.y - ball.rad, 2 * ball.rad, 2 * ball.rad, ball.hBitmap, true);// �����
}

void LimitRacket()
{
	racket.x = max(racket.x, racket.width / 2.);//���� ��������� ������ ���� ������� ������ ����, �������� �� ����
	racket.x = min(racket.x, window.width - racket.width / 2.);//���������� ��� ������� ����
}

void CheckWalls()
{
	if (ball.x < ball.rad || ball.x > window.width - ball.rad)
	{
		ball.dx *= -1;
		//ProcessSound("bounce.wav");
	}
}

void CheckRoof()
{
	if (ball.y < ball.rad)
	{
		ball.dy *= -1;
		//ProcessSound("bounce.wav");
	}
}

void CheckBlock()
{
	for (int i = 0; i < brickColumn; i++)
	{
		for (int j = 0; j < brickRow; j++)
		{
			int borderX = brickArray[j][i].width + brickArray[j][i].x;
			int borderY = brickArray[j][i].height + brickArray[j][i].y;
			if (ball.x >= brickArray[j][i].x && ball.x <= borderX && ball.y >= brickArray[j][i].y && ball.y <= borderY && brickArray[j][i].status)
			{
				brickArray[j][i].status = false;
				int minX = min(ball.x - brickArray[j][i].x, borderX - ball.x);
				int minY = min(ball.y - brickArray[j][i].y, borderY - ball.y);

				if (minY < minX)
				{
					ball.dy *= -1;
					return;
				}
				else {
					ball.dx *= -1;
					return;
				}

			}
		}
	}
}

bool tail = false;

void CheckFloor()
{
	if (ball.y > window.height - ball.rad - racket.height)//����� ������� ����� ������� - ����������� �������
	{
		if (!tail && ball.x >= racket.x - racket.width / 2. - ball.rad && ball.x <= racket.x + racket.width / 2. + ball.rad)//����� �����, � �� �� � ������ ��������� ������
		{
			game.score++;//�� ������ ������� ���� ���� ����
			ball.speed += 5. / game.score;//�� ����������� ��������� - ���������� �������� ������
			ball.dy *= -1;//������
			racket.width -= 10. / game.score;//������������� ��������� ������ ������� - ��� ���������
			//ProcessSound("bounce.wav");//������ ���� �������
		}
		else
		{//����� �� �����

			tail = true;//����� ������ ������ ���� �������

			if (ball.y - ball.rad > window.height)//���� ����� ���� �� ������� ����
			{
				game.balls--;//��������� ���������� "������"

				//ProcessSound("fail.wav");//������ ����

				if (game.balls < 0) { //�������� ������� ��������� "������"

					MessageBoxA(window.hWnd, "game over", "", MB_OK);//������� ��������� � ���������
					InitGame();//������������������ ����
				}

				ball.dy = (rand() % 65 + 35) / 100.;//������ ����� ��������� ������ ��� ������
				ball.dx = -(1 - ball.dy);
				ball.x = racket.x;//�������������� ���������� ������ - ������ ��� �� �������
				ball.y = racket.y - ball.rad;
				game.action = false;//���������������� ����, ���� ����� �� ������ ������
				tail = false;
			}
		}
	}
}

void ProcessRoom()
{
	//������������ �����, ������� � ���. ������� - ���� ������� ����� ���� ���������, � ������, ��� ������� �� ����� ������ ������������� ����� ������� �������� ������
	CheckWalls();
	CheckRoof();
	CheckFloor();
	CheckBlock();
	ShowBricks();

}

void ProcessBall()
{
	if (game.action)
	{
		//���� ���� � �������� ������ - ���������� �����
		ball.x += ball.dx * ball.speed;
		ball.y += ball.dy * ball.speed;
		Trace.x = ball.x;
		Trace.y = ball.y;
	}
	else
	{
		//����� - ����� "��������" � �������
		ball.x = racket.x;
		Trace.x = ball.x;
	}
}

void InitWindow()
{
	SetProcessDPIAware();
	window.hWnd = CreateWindow("edit", 0, WS_POPUP | WS_VISIBLE | WS_MAXIMIZE, 0, 0, 0, 0, 0, 0, 0, 0);

	RECT r;
	GetClientRect(window.hWnd, &r);
	window.device_context = GetDC(window.hWnd);//�� ������ ���� ������� ����� ��������� ���������� ��� ���������
	window.width = r.right - r.left;//���������� ������� � ���������
	window.height = r.bottom - r.top;
	window.context = CreateCompatibleDC(window.device_context);//������ �����
	SelectObject(window.context, CreateCompatibleBitmap(window.device_context, window.width, window.height));//����������� ���� � ���������
	GetClientRect(window.hWnd, &r);

}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{

	InitWindow();//����� �������������� ��� ��� ����� ��� ��������� � ����
	InitGame();//����� �������������� ���������� ����

	//mciSendString(TEXT("play ..\\Debug\\music.mp3 repeat"), NULL, 0, NULL);
	ShowCursor(NULL);

	while (!GetAsyncKeyState(VK_ESCAPE))
	{
		ShowRacketAndBall();//������ ���, ������� � �����
		ShowScore();//������ ���� � �����

		Sleep(16);//���� 16 ���������� (1/���������� ������ � �������)

		ProcessInput();//����� ����������
		LimitRacket();//���������, ����� ������� �� ������� �� �����
		ShowTrace();
		ProcessBall();//���������� �����        
		ProcessRoom();//������������ ������� �� ���� � �������, ��������� ������ � ��������
		BitBlt(window.device_context, 0, 0, window.width, window.height, window.context, 0, 0, SRCCOPY);//�������� ����� � ����
	}

}
