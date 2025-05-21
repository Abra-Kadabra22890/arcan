//linker::system::subsystem  - Windows(/ SUBSYSTEM:WINDOWS)
//configuration::advanced::character set - not set
//linker::input::additional dependensies Msimg32.lib; Winmm.lib

#include "windows.h"
#include <cmath>

// секция данных игры  
typedef struct {
	float width, height, x, y, rad, dx, dy, speed;
	HBITMAP hBitmap;//хэндл к спрайту шарика 
	bool status;
} sprite;

sprite racket;//ракетка игрока
sprite enemy;//ракетка противника
sprite ball;//шарик
sprite Trace;

const int brickRow = 20;
const int brickColumn = 5;

sprite brickArray[brickRow][brickColumn];

struct {
	int score, balls;//количество набранных очков и оставшихся "жизней"
	bool action = false;//состояние - ожидание (игрок должен нажать пробел) или игра
} game;

struct {
	HWND hWnd;//хэндл окна
	HDC device_context, context;// два контекста устройства (для буферизации)
	int width, height;//сюда сохраним размеры окна которое создаст программа
} window;

struct {
	int x;
	int y;
} coordinate;

HBITMAP hBack;// хэндл для фонового изображения

//cекция кода

void InitGame()
{
	//в этой секции загружаем спрайты с помощью функций gdi
	//пути относительные - файлы должны лежать рядом с .exe 
	//результат работы LoadImageA сохраняет в хэндлах битмапов, рисование спрайтов будет произовдиться с помощью этих хэндлов
	ball.hBitmap = (HBITMAP)LoadImageA(NULL, "ball.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	racket.hBitmap = (HBITMAP)LoadImageA(NULL, "racket.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	enemy.hBitmap = (HBITMAP)LoadImageA(NULL, "racket_enemy.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	hBack = (HBITMAP)LoadImageA(NULL, "back.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	//------------------------------------------------------

	racket.width = 300;
	racket.height = 50;
	racket.speed = 30;//скорость перемещения ракетки
	racket.x = window.width / 2.;//ракетка посередине окна
	racket.y = window.height - racket.height;//чуть выше низа экрана - на высоту ракетки

	ball.dy = (rand() % 65 + 35) / 100.;//формируем вектор полета шарика
	ball.dx = -(1 - ball.dy);//формируем вектор полета шарика
	ball.speed = 11;
	ball.rad = 20;
	ball.x = racket.x;//x координата шарика - на середие ракетки
	ball.y = racket.y - ball.rad;//шарик лежит сверху ракетки
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

void ProcessSound(const char* name)//проигрывание аудиофайла в формате .wav, файл должен лежать в той же папке где и программа
{
	PlaySound(TEXT(name), NULL, SND_FILENAME | SND_ASYNC);//переменная name содежрит имя файла. флаг ASYNC позволяет проигрывать звук паралельно с исполнением программы
}

void ShowScore()
{
	//поиграем шрифтами и цветами
	SetTextColor(window.context, RGB(160, 160, 160));
	SetBkColor(window.context, RGB(0, 0, 0));
	SetBkMode(window.context, TRANSPARENT);
	auto hFont = CreateFont(70, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 2, 0, "CALIBRI");
	auto hTmp = (HFONT)SelectObject(window.context, hFont);

	char txt[32];//буфер для текста
	_itoa_s(game.score, txt, 10);//преобразование числовой переменной в текст. текст окажется в переменной txt
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

	hMemDC = CreateCompatibleDC(hDC); // Создаем контекст памяти, совместимый с контекстом отображения
	hOldbm = (HBITMAP)SelectObject(hMemDC, hBitmapBall);// Выбираем изображение bitmap в контекст памяти

	if (hOldbm) // Если не было ошибок, продолжаем работу
	{
		GetObject(hBitmapBall, sizeof(BITMAP), (LPSTR)&bm); // Определяем размеры изображения

		if (alpha)
		{
			TransparentBlt(window.context, x, y, x1, y1, hMemDC, 0, 0, x1, y1, RGB(0, 0, 0));//все пиксели черного цвета будут интепретированы как прозрачные
		}
		else
		{
			StretchBlt(hDC, x, y, x1, y1, hMemDC, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY); // Рисуем изображение bitmap
		}

		SelectObject(hMemDC, hOldbm);// Восстанавливаем контекст памяти
	}

	DeleteDC(hMemDC); // Удаляем контекст памяти
}

void ShowBricks()
{
	for (int i = 0; i < brickColumn; i++)
	{
		for (int j = 0; j < brickRow; j++)
		{
			if (brickArray[j][i].status)
			{
				ShowBitmap(window.context, brickArray[j][i].x, brickArray[j][i].y, brickArray[j][i].width, brickArray[j][i].height, brickArray[j][i].hBitmap);//ракетка оппонента
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
	if (y > window.height - ball.rad - racket.height)//шарик пересек линию отскока - горизонталь ракетки
	{
		if (x >= x - racket.width / 2. - ball.rad && x <= racket.x + racket.width / 2. + ball.rad)//шарик отбит, и мы не в режиме обработки хвоста
		{
			Trace.dy *= -1;//отскок
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
	ShowBitmap(window.context, 0, 0, window.width, window.height, hBack);//задний фон
	ShowBitmap(window.context, racket.x - racket.width / 2., racket.y, racket.width, racket.height, racket.hBitmap);// ракетка игрока  
	ShowBricks();
	ShowBitmap(window.context, ball.x - ball.rad, ball.y - ball.rad, 2 * ball.rad, 2 * ball.rad, ball.hBitmap, true);// шарик
}

void LimitRacket()
{
	racket.x = max(racket.x, racket.width / 2.);//если коодината левого угла ракетки меньше нуля, присвоим ей ноль
	racket.x = min(racket.x, window.width - racket.width / 2.);//аналогично для правого угла
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
	if (ball.y > window.height - ball.rad - racket.height)//шарик пересек линию отскока - горизонталь ракетки
	{
		if (!tail && ball.x >= racket.x - racket.width / 2. - ball.rad && ball.x <= racket.x + racket.width / 2. + ball.rad)//шарик отбит, и мы не в режиме обработки хвоста
		{
			game.score++;//за каждое отбитие даем одно очко
			ball.speed += 5. / game.score;//но увеличиваем сложность - прибавляем скорости шарику
			ball.dy *= -1;//отскок
			racket.width -= 10. / game.score;//дополнительно уменьшаем ширину ракетки - для сложности
			//ProcessSound("bounce.wav");//играем звук отскока
		}
		else
		{//шарик не отбит

			tail = true;//дадим шарику упасть ниже ракетки

			if (ball.y - ball.rad > window.height)//если шарик ушел за пределы окна
			{
				game.balls--;//уменьшаем количество "жизней"

				//ProcessSound("fail.wav");//играем звук

				if (game.balls < 0) { //проверка условия окончания "жизней"

					MessageBoxA(window.hWnd, "game over", "", MB_OK);//выводим сообщение о проигрыше
					InitGame();//переинициализируем игру
				}

				ball.dy = (rand() % 65 + 35) / 100.;//задаем новый случайный вектор для шарика
				ball.dx = -(1 - ball.dy);
				ball.x = racket.x;//инициализируем координаты шарика - ставим его на ракетку
				ball.y = racket.y - ball.rad;
				game.action = false;//приостанавливаем игру, пока игрок не нажмет пробел
				tail = false;
			}
		}
	}
}

void ProcessRoom()
{
	//обрабатываем стены, потолок и пол. принцип - угол падения равен углу отражения, а значит, для отскока мы можем просто инвертировать часть вектора движения шарика
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
		//если игра в активном режиме - перемещаем шарик
		ball.x += ball.dx * ball.speed;
		ball.y += ball.dy * ball.speed;
		Trace.x = ball.x;
		Trace.y = ball.y;
	}
	else
	{
		//иначе - шарик "приклеен" к ракетке
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
	window.device_context = GetDC(window.hWnd);//из хэндла окна достаем хэндл контекста устройства для рисования
	window.width = r.right - r.left;//определяем размеры и сохраняем
	window.height = r.bottom - r.top;
	window.context = CreateCompatibleDC(window.device_context);//второй буфер
	SelectObject(window.context, CreateCompatibleBitmap(window.device_context, window.width, window.height));//привязываем окно к контексту
	GetClientRect(window.hWnd, &r);

}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{

	InitWindow();//здесь инициализируем все что нужно для рисования в окне
	InitGame();//здесь инициализируем переменные игры

	//mciSendString(TEXT("play ..\\Debug\\music.mp3 repeat"), NULL, 0, NULL);
	ShowCursor(NULL);

	while (!GetAsyncKeyState(VK_ESCAPE))
	{
		ShowRacketAndBall();//рисуем фон, ракетку и шарик
		ShowScore();//рисуем очик и жизни

		Sleep(16);//ждем 16 милисекунд (1/количество кадров в секунду)

		ProcessInput();//опрос клавиатуры
		LimitRacket();//проверяем, чтобы ракетка не убежала за экран
		ShowTrace();
		ProcessBall();//перемещаем шарик        
		ProcessRoom();//обрабатываем отскоки от стен и каретки, попадание шарика в картетку
		BitBlt(window.device_context, 0, 0, window.width, window.height, window.context, 0, 0, SRCCOPY);//копируем буфер в окно
	}

}
