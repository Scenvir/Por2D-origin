#include <stdio.h>
#include <graphics.h>
#include <easyx.h>
#include <conio.h>
#define blsize 20
#define hitsize 10
#define reveal_portal_direction 1//调试/作弊可用
//重力加速度为恒为1，因而未使用定义
int temp;
int cx, cy, cvx, cvy, cax;//角色的位置（最左上角的点），速度和加速度
int block[50][30] = { 0 };//地图方块的位置
int hitpoint[102][62];//地图方块的碰撞判定点
int is_on_ground = 1;//在地面上的判定，角色不受重力加速度
int head_touch, l_touch, r_touch;//碰到头顶，左侧，右侧
int chsize[] = { 18,58 };//角色碰撞箱
int chd = 0;//角色方向
int pfd[2], ptd[2];//传送门的两个朝向：面向和指向
int px, py; //角色在另一扇传送门中的位置
int ptx[2], pty[2];//传送门的点位，角色相对于其的位置、速度会被映射到另一扇门上
int can_draw;//是否绘出另一扇传送门中的角色
int canshoot = 0;//避免连射传送门
/* i need to explain here.
 * we dissolve our portal into is own direction and its facing direction.
 * we shall find the 'top' point of the portal.
 * in facing direction the sum of the 2 characters is equal to its size.
 * in portal direction the same length of the 2 characters is equal to the size.
 * with these we can find the px and py.
 * we can define the ptd change by listing all of them.
 * then we should be able to discribe the position and direction of our portal character.
 * by analyze the hit of the portal character we limit the movement of the real character.
 * when the character half-crossed the portal we change the real one and the portal one.
 * hope this kind of logic will work.
 */
 //不翻译了，看个乐呵
int is_in_portal = -1;//-1不在门中，0在0门中（蓝门），1在1门中（橙门），以角色中心点为判断
int headx, heady, centerx, centery;//头的判定点和身体中心的判定点
int reachx, reachy;//角色最右下角的点
int pd;//传送门中角色投影方向
int tele = 0;//避免连续传送
int d_flag;//检测人物方向是否应为门中投影方向
int endx, endy, endd;//过关之门的位置和方向
struct portal_struct
{
	int ptx;
	int pty;
	int ptd;
}
portal[2];//门的属性
void level_reset()//重置关卡
{
	for (int i = 0; i < 2; i++)
	{
		portal[i].ptd = 0;
		portal[i].ptx = 0;
		portal[i].pty = 0;
	}
	chd = 0;
	cvx = 0;
	cvy = 0;
	for (int i = 0; i < 50; i++)
		for (int j = 0; j < 30; j++)
			block[i][j] = 0;
	//	d_flag = 0;//作为刻意设计的特性，d_flag在重开时不会重置，这是关卡“重生”的设想
}
int trans(int x, int y, int is_in_portal, int to_return)//从门is_in_portal传送到另一扇门的速度位置变化
{
	int df, dp;
	int dfv, dpv;
	int pcx, pcy;
	int pvx, pvy;//全为传送时使用的中间变量
	if (is_in_portal >= 0)
	{
		switch (pfd[is_in_portal])
		{
		case 0:
			df = pty[is_in_portal] - y;
			dfv = y;
			break;
		case 1:
			df = x - ptx[is_in_portal];
			dfv = -x;
			break;
		case 2:
			df = y - pty[is_in_portal];
			dfv = -y;
			break;
		case 3:
			df = ptx[is_in_portal] - x;
			dfv = x;
			break;
		}
		switch (ptd[is_in_portal])
		{
		case 0:
			dp = y - pty[is_in_portal];
			dpv = -y;
			break;
		case 1:
			dp = ptx[is_in_portal] - x;
			dpv = x;
			break;
		case 2:
			dp = pty[is_in_portal] - y;
			dpv = y;
			break;
		case 3:
			dp = x - ptx[is_in_portal];
			dpv = -x;
			break;
		}
		switch (pfd[1 - is_in_portal])
		{
		case 0:
			pcy = pty[1 - is_in_portal] + df;
			pvy = -dfv;
			break;
		case 1:
			pcx = ptx[1 - is_in_portal] - df;
			pvx = dfv;
			break;
		case 2:
			pcy = pty[1 - is_in_portal] - df;
			pvy = dfv;
			break;
		case 3:
			pcx = ptx[1 - is_in_portal] + df;
			pvx = -dfv;
			break;
		}
		switch (ptd[1 - is_in_portal])
		{
		case 0:
			pcy = pty[1 - is_in_portal] + dp;
			pvy = -dpv;
			break;
		case 1:
			pcx = ptx[1 - is_in_portal] - dp;
			pvx = dpv;
			break;
		case 2:
			pcy = pty[1 - is_in_portal] - dp;
			pvy = dpv;
			break;
		case 3:
			pcx = ptx[1 - is_in_portal] + dp;
			pvx = -dpv;
			break;
		}
	}
	if (to_return == 0)
		return pcx;
	if (to_return == 1)
		return pcy;
	if (to_return == 2)
		return pvx;
	if (to_return == 3)
		return pvy;//不同种类的变换被整合到同一函数中，用to_return区分
}
void reload_position()//加载角色在另一扇门中的投影
{
	is_in_portal = -1;
	if (tele > 0)
		tele--;
	headx = cx + chsize[chd & 1 - (chd == 3)] - 9;//此后还有许多位运算，可以省下一些判断语句
	heady = cy + chsize[(chd & 2) / 2 - (chd == 3)] - 9;
	centerx = cx + chsize[chd & 1] / 2;
	centery = cy + chsize[1 - (chd & 1)] / 2;
	reachx = cx + chsize[chd & 1];
	reachy = cy + chsize[1 - (chd & 1)];//更新角色头、中心、右下角点的位置
	for (int i = 0; i < 2; i++)
	{
		int d = portal[i].ptd - 1;
		ptx[i] = (portal[i].ptx + (1 - (d & 1)) * (d & 2) / 2 * 3 + (1 - (d & 2) / 2) * (d & 4) / 4) * blsize;
		pty[i] = (portal[i].pty + (2 - (d & 2)) * (d & 1) / 2 * 3 + (d & 2) * (d & 4) / 8) * blsize;
		pfd[i] = (d >> 1) - 1 & 3;
		ptd[i] = 2 * (d & 3 & 1) + (d & 3 & 2) / 2;//解码门的信息
		switch (pfd[i])//在门中的判定
		{
		case 0:
			if (cx >= portal[i].ptx * blsize && cx<(portal[i].ptx + 3) * blsize && reachx > portal[i].ptx * blsize && reachx <= (portal[i].ptx + 3) * blsize && pty[i] < reachy ^ pty[i] < cy - 15)////
			{
				is_in_portal = i;
			}
			break;
		case 1:
			if (cy >= portal[i].pty * blsize && cy < (portal[i].pty + 3) * blsize && reachy > portal[i].pty * blsize && reachy <= (portal[i].pty + 3) * blsize && ptx[i] < reachx ^ ptx[i] < cx)
			{
				is_in_portal = i;
			}
			break;
		case 2:
			if (cx >= portal[i].ptx * blsize && cx < (portal[i].ptx + 3) * blsize && reachx > portal[i].ptx * blsize && reachx <= (portal[i].ptx + 3) * blsize && pty[i] < reachy ^ pty[i] < cy)
			{
				is_in_portal = i;
			}
			break;
		case 3:
			if (cy >= portal[i].pty * blsize && cy<(portal[i].pty + 3) * blsize && reachy > portal[i].pty * blsize && reachy <= (portal[i].pty + 3) * blsize && ptx[i] < reachx ^ ptx[i] < cx)
			{
				is_in_portal = i;
			}
			break;
		}
	}
	//保证重加载
	if (is_on_ground)
		d_flag = 0;//
	if (is_in_portal >= 0)//处于某一门中才会进行以下计算
	{
		can_draw = 1;//允许绘制投影
		d_flag = 0;//重置角色方向取向选择
		px = min(trans(cx, cy, is_in_portal, 0), trans(reachx, reachy, is_in_portal, 0));
		py = min(trans(cx, cy, is_in_portal, 1), trans(reachx, reachy, is_in_portal, 1));
		int pheadx = trans(headx, heady, is_in_portal, 0);
		int pheady = trans(headx, heady, is_in_portal, 1);
		int pcenterx = trans(centerx, centery, is_in_portal, 0);
		int pcentery = trans(centerx, centery, is_in_portal, 1);//更新角色的位置和速度投影
		if (pheady - pcentery > 1)
			pd = 2;
		if (pheady - pcentery < -1)
			pd = 0;
		if (pheadx - pcenterx > 1)
			pd = 1;
		if (pheadx - pcenterx < -1)
			pd = 3;//投影的方向计算
		switch (pfd[is_in_portal])//将角色传送至另一个门的投影处，相对位置和速度保持不变
		{
		case 0:
			if (!tele && pty[is_in_portal] < centery)
			{
				tele = 2;
				cx = px;
				cy = py;
				chd = pd;
				int temp1 = cvx, temp2 = cvy;
				cvx = trans(temp1, temp2, is_in_portal, 2);
				cvy = trans(temp1, temp2, is_in_portal, 3);
			}
			if (pty[is_in_portal] < heady)
			{
				d_flag = 1;
				headx = pheadx;
				heady = pheady;
			}
			break;
		case 1:
			if (!tele && ptx[is_in_portal] > centerx)
			{
				tele = 2;
				cx = px;
				cy = py;
				chd = pd;
				int temp1 = cvx, temp2 = cvy;
				cvx = trans(temp1, temp2, is_in_portal, 2);
				cvy = trans(temp1, temp2, is_in_portal, 3);
			}
			if (ptx[is_in_portal] > headx)
			{
				d_flag = 1;
				headx = pheadx;
				heady = pheady;
			}
			break;
		case 2:
			if (!tele && pty[is_in_portal] > centery)
			{
				tele = 2;
				cx = px;
				cy = py;
				chd = pd;
				int temp1 = cvx, temp2 = cvy;
				cvx = trans(temp1, temp2, is_in_portal, 2);
				cvy = trans(temp1, temp2, is_in_portal, 3);
			}
			if (pty[is_in_portal] > heady)
			{
				d_flag = 1;
				headx = pheadx;
				heady = pheady;
			}
			break;
		case 3:
			if (!tele && ptx[is_in_portal] < centerx)
			{
				tele = 2;
				cx = px;
				cy = py;
				chd = pd;
				int temp1 = cvx, temp2 = cvy;
				cvx = trans(temp1, temp2, is_in_portal, 2);
				cvy = trans(temp1, temp2, is_in_portal, 3);
			}
			if (ptx[is_in_portal] < headx)
			{
				d_flag = 1;
				headx = pheadx;
				heady = pheady;
			}
			break;
		}
	}
	if (reveal_portal_direction)//显示传送门朝向
	{
		setfillcolor(RGB(0, 255, 0));
		solidrectangle(ptx[0], pty[0], ptx[0] + 5, pty[0] + 5);
		solidrectangle(ptx[1], pty[1], ptx[1] + 5, pty[1] + 5);
	}
}
void mpr(int type, int x1, int y1, int x2, int y2)//地图编辑器，将一个矩形填充为方块type
{
	for (int i = x1; i <= x2; i++)
		for (int j = y1; j <= y2; j++)
			block[i][j] = type;
}
ExMessage msg;//鼠标信息
void drawreset()//重置碰撞逻辑
{
	cax = 0;
	is_on_ground = 0;
	head_touch = 0;
	l_touch = 0;
	r_touch = 0;
	for (int i = 0; i < 100; i++)
		for (int j = 0; j < 60; j++)
			hitpoint[i][j] = 0;
}
void load_hitbox();//引用一下
void detect1()//实装的一种碰撞检测方式
{
	//编码碰撞点属性：1存在，2上，4右，8下，16左
	for (int i = 1; i < 100; i++)
		for (int j = 1; j < 60; j++)
		{
			if (hitpoint[i][j])
			{
				if (!hitpoint[i][j + 1])
				{
					hitpoint[i][j] += 2;
				}
				if (!hitpoint[i - 1][j])
				{
					hitpoint[i][j] += 4;
				}
				if (!hitpoint[i][j - 1])
				{
					hitpoint[i][j] += 8;
				}
				if (!hitpoint[i + 1][j])
				{
					hitpoint[i][j] += 16;
				}
			}
		}
	//四个方向上的碰撞计算
	for (int i = (cx + 2) / hitsize + 1; i <= (cx + chsize[chd & 1] - 4) / hitsize; i++)
		if (hitpoint[i][(cy + chsize[1 - chd & 1]) / blsize * 2] >> 3 & 1)
		{
			is_on_ground = 1;
		}
	if (is_on_ground)
	{
		cy -= cy % blsize / 2;
		cvy = 0;
	}
	for (int i = (cx + 2) / hitsize + 1; i <= (cx + chsize[chd & 1] - 4) / hitsize; i++)
		if (hitpoint[i][cy / blsize * 2 + 2] >> 1 & 1)
		{
			head_touch = 1;
		}
	if (head_touch) {
		cy += (blsize - cy % blsize) / 2;
		cvy = 0;
	}
	for (int i = cy / hitsize + 1; i <= (cy + chsize[1 - chd & 1] - 1) / hitsize; i++)
		if (hitpoint[(cx + chsize[chd & 1]) / blsize * 2][i] >> 2 & 1)
		{
			if (i == cy / hitsize + 1 && (!head_touch || is_on_ground) || i == (cy + chsize[1 - chd & 1] - 1) / hitsize && !is_on_ground || (i != cy / hitsize + 1) && (i != (cy + chsize[1 - chd & 1] - 1) / hitsize))
				r_touch = 1;
		}
	if (r_touch)
	{
		cx -= cx % blsize / 2;
		cvx = 0;
	}
	for (int i = cy / hitsize + 1; i <= (cy + chsize[1 - chd & 1] - 1) / hitsize; i++)
		if (hitpoint[cx / blsize * 2 + 2][i] >> 4 & 1)
		{
			if (i == cy / hitsize + 1 && (!head_touch || is_on_ground) || i == (cy + chsize[1 - chd & 1] - 1) / hitsize && !is_on_ground || (i != cy / hitsize + 1) && (i != (cy + chsize[1 - chd & 1] - 1) / hitsize))
				l_touch = 1;
		}
	if (l_touch)
	{
		cx += (-cx % hitsize + hitsize);
		cvx = 0;
	}
}
void load_hitbox()//依据地图方块加载碰撞点
{
	for (int i = 0; i < 50; i++)
	{
		for (int j = 0; j < 30; j++)
		{
			if (block[i][j])
			{
				for (int k = 0; k < 3; k++)
					for (int l = 0; l < 3; l++)
						hitpoint[2 * i + k][2 * j + l] = 1;
			}
		}
	}
}
void move()//角色移动
{
	cy += cvy;
	cx += cvx;
	detect1();
	if (GetAsyncKeyState('D'))
	{
		cax = 1;
	}
	if (GetAsyncKeyState('A'))
	{
		cax = -1;
	}
	if (GetAsyncKeyState('W'))//跳跃给初速度
	{
		if (is_on_ground)
		{
			cvy = -7;
			is_on_ground = 0;
		}
	}
	if (cvx > -6 && cvx < 6)
		cvx += cax;//限制移动速度
	else cvx *= 0.9;//过高速度的空气阻力（
	if (!GetAsyncKeyState('D') && !GetAsyncKeyState('A') && is_on_ground) cvx *= 0.5;//地面阻力
	if (!is_on_ground && cvy < 20)cvy += 1;//这个1就是重力加速度
}
void portal_use()//传送门的使用
{
	setfillcolor(RGB(255, 0, 0));
	solidrectangle(headx, heady, headx + 3, heady + 3);//标记角色的头判定点，后续可能会被替换
	int mousex = msg.x;
	int mousey = msg.y;
	if (portal[0].ptd)//解算并绘制门
	{
		setfillcolor(RGB(50, 150, 255));
		for (int i = 0; i < 3; i++)
		{
			block[portal[0].ptx + i * (portal[0].ptd - 1 & 2) / 2][portal[0].pty + i * (1 - (portal[0].ptd - 1 & 2) / 2)] = -1;
			solidrectangle(blsize * (portal[0].ptx + i * (portal[0].ptd - 1 & 2) / 2) + 1, blsize * (portal[0].pty + i * (1 - (portal[0].ptd - 1 & 2) / 2)) + 1, blsize * (portal[0].ptx + i * (portal[0].ptd - 1 & 2) / 2) + 19, blsize * (portal[0].pty + i * (1 - (portal[0].ptd - 1 & 2) / 2)) + 19);
		}
	}
	if (portal[1].ptd)
	{
		setfillcolor(RGB(255, 150, 50));
		for (int i = 0; i < 3; i++)
		{
			block[portal[1].ptx + i * (portal[1].ptd - 1 & 2) / 2][portal[1].pty + i * (1 - (portal[1].ptd - 1 & 2) / 2)] = -2;
			solidrectangle(blsize * (portal[1].ptx + i * (portal[1].ptd - 1 & 2) / 2) + 1, blsize * (portal[1].pty + i * (1 - (portal[1].ptd - 1 & 2) / 2)) + 1, blsize * (portal[1].ptx + i * (portal[1].ptd - 1 & 2) / 2) + 19, blsize * (portal[1].pty + i * (1 - (portal[1].ptd - 1 & 2) / 2)) + 19);
		}
	}
	while (peekmessage(&msg, EM_MOUSE));//检测鼠标信息，准备射门
	{
		switch (msg.message)
		{
		case WM_LBUTTONDOWN:case WM_RBUTTONDOWN:
			canshoot = 1;//鼠标抬起射门，避免瞬间多次射门
			break;
		case WM_LBUTTONUP://左键射门（蓝门），后面右键射门（橙门）逻辑一样
			if (canshoot && is_in_portal)//蓝门，注意在角色身处蓝门中时无法射出蓝门，橙门同理；角色无法在蓝门存在的方块上开橙门，但可以在这附近的方块上重射蓝门以改变其方向和位置，反之亦然
			{
				canshoot = 0;
				setlinecolor(RGB(50, 150, 255));
				setlinestyle(PS_SOLID | PS_ENDCAP_FLAT, 3);//射门的激光（
				float k = (float)(msg.y - heady) / (float)(msg.x - headx);
				if (k > 20.0)
					k = 20.0;
				if (k < -20.0)
					k = -20.0;//避免斜率过大
				int hitwall;//激光撞墙检测
				if (msg.x > headx)
					for (hitwall = 0; !block[(headx + hitwall) / blsize][(heady + (int)(k * hitwall)) / blsize]; hitwall += 1)
					{
						line(headx + hitwall, heady + hitwall * k, headx + hitwall + 1, heady + (hitwall + 1) * k);
					}
				else
				{
					for (hitwall = 0; !block[(headx + hitwall) / blsize][(heady + (int)(k * hitwall)) / blsize]; hitwall -= 1)
					{
						line(headx + hitwall, heady + hitwall * k, headx + hitwall - 1, heady + (hitwall - 1) * k);
					}
				}
				mousex = headx + hitwall;
				mousey = heady + (int)(k * hitwall);//门出现的点
				if ((block[mousex / blsize - 1][mousey / blsize] == 1 || block[mousex / blsize - 1][mousey / blsize] == -1) && (block[mousex / blsize][mousey / blsize] == 1 || block[mousex / blsize][mousey / blsize] == -1) && (block[mousex / blsize + 1][mousey / blsize] == 1 || block[mousex / blsize + 1][mousey / blsize] == -1))//门左右有可射门方块
				{
					int reald = chd;
					if (d_flag)
						reald = pd;
					switch (reald)//决定门的种类，与角色朝向，墙面朝向（上面的if决定）和开门的激光射线斜率有关
						//一般来说，不考虑重生的情况下，有角色头部判定点H，中心点C，射门点P，门的方向即CP×HC×PH在墙上的投影，所有门的朝向都为该逻辑
					{
					case 0:
						if (k > 0)
						{
							if (mousey > heady)
								portal[0].ptd = 3;
							else
								portal[0].ptd = 7;
						}
						else
						{
							if (mousey > heady)
								portal[0].ptd = 4;
							else
								portal[0].ptd = 8;
						}
						break;
					case 1:
						if (mousey > heady)
							portal[0].ptd = 3;
						else
							portal[0].ptd = 7;
						break;
					case 2:
						if (k > 0)
						{
							if (mousey > heady)
								portal[0].ptd = 4;
							else
								portal[0].ptd = 8;
						}
						else
						{
							if (mousey > heady)
								portal[0].ptd = 3;
							else
								portal[0].ptd = 7;
						}
						break;
					case 3:
						if (mousey > heady)
							portal[0].ptd = 4;
						else
							portal[0].ptd = 8;
					}
					portal[0].ptx = mousex / blsize - 1;
					portal[0].pty = mousey / blsize;//门的左上角点的位置
				}
				if ((block[mousex / blsize][mousey / blsize - 1] == 1 || block[mousex / blsize][mousey / blsize - 1] == -1) && (block[mousex / blsize][mousey / blsize] == 1 || block[mousex / blsize][mousey / blsize] == -1) && (block[mousex / blsize][mousey / blsize + 1] == 1 || block[mousex / blsize][mousey / blsize + 1] == -1))//门上下有可射门方块
				{
					int reald = chd;
					if (d_flag)
						reald = pd;
					switch (reald)
					{
					case 0:
						if (mousex > headx)
							portal[0].ptd = 1;
						else
							portal[0].ptd = 5;
						break;
					case 1:
						if (k > 0)
						{
							if (mousex > headx)
								portal[0].ptd = 1;
							else
								portal[0].ptd = 5;
						}
						else
						{
							if (mousex > headx)
								portal[0].ptd = 2;
							else
								portal[0].ptd = 6;
						}
						break;
					case 2:
						if (mousex > headx)
							portal[0].ptd = 2;
						else
							portal[0].ptd = 6;
						break;
					case 3:
						if (k > 0)
						{
							if (mousex > headx)
								portal[0].ptd = 2;
							else
								portal[0].ptd = 6;
						}
						else
						{
							if (mousex > headx)
								portal[0].ptd = 1;
							else
								portal[0].ptd = 5;
						}
					}
					portal[0].ptx = mousex / blsize;
					portal[0].pty = mousey / blsize - 1;
				}
			}
			break;
		case WM_RBUTTONUP://橙门，逻辑一致
			if (canshoot && is_in_portal != 1)
			{
				canshoot = 0;
				setlinecolor(RGB(255, 150, 50));
				setlinestyle(PS_SOLID | PS_ENDCAP_FLAT, 3);
				float k = (float)(msg.y - heady) / (float)(msg.x - headx);
				if (k > 20.0)
					k = 20.0;
				if (k < -20.0)
					k = -20.0;
				int hitwall;
				if (msg.x > headx)
					for (hitwall = 0; !block[(headx + hitwall) / blsize][(heady + (int)(k * hitwall)) / blsize]; hitwall += 1)
					{
						line(headx + hitwall, heady + hitwall * k, headx + hitwall + 1, heady + (hitwall + 1) * k);
					}
				else
				{
					for (hitwall = 0; !block[(headx + hitwall) / blsize][(heady + (int)(k * hitwall)) / blsize]; hitwall -= 1)
					{
						line(headx + hitwall, heady + hitwall * k, headx + hitwall - 1, heady + (hitwall - 1) * k);
					}
				}
				mousex = headx + hitwall;
				mousey = heady + (int)(k * hitwall);
				if ((block[mousex / blsize - 1][mousey / blsize] == 1 || block[mousex / blsize - 1][mousey / blsize] == -2) && (block[mousex / blsize][mousey / blsize] == 1 || block[mousex / blsize][mousey / blsize] == -2) && (block[mousex / blsize + 1][mousey / blsize] == 1 || block[mousex / blsize + 1][mousey / blsize] == -2))
				{
					int reald = chd;
					if (d_flag)
						reald = pd;
					switch (reald)
					{
					case 0:
						if (k > 0)
						{
							if (mousey > heady)
								portal[1].ptd = 3;
							else
								portal[1].ptd = 7;
						}
						else
						{
							if (mousey > heady)
								portal[1].ptd = 4;
							else
								portal[1].ptd = 8;
						}
						break;
					case 1:
						if (mousey > heady)
							portal[1].ptd = 3;
						else
							portal[1].ptd = 7;
						break;
					case 2:
						if (k > 0)
						{
							if (mousey > heady)
								portal[1].ptd = 4;
							else
								portal[1].ptd = 8;
						}
						else
						{
							if (mousey > heady)
								portal[1].ptd = 3;
							else
								portal[1].ptd = 7;
						}
						break;
					case 3:
						if (mousey > heady)
							portal[1].ptd = 4;
						else
							portal[1].ptd = 8;
					}
					portal[1].ptx = mousex / blsize - 1;
					portal[1].pty = mousey / blsize;
				}
				if ((block[mousex / blsize][mousey / blsize - 1] == 1 || block[mousex / blsize][mousey / blsize - 1] == -2) && (block[mousex / blsize][mousey / blsize] == 1 || block[mousex / blsize][mousey / blsize] == -2) && (block[mousex / blsize][mousey / blsize + 1] == 1 || block[mousex / blsize][mousey / blsize + 1] == -2))
				{
					int reald = chd;
					if (d_flag)
						reald = pd;
					switch (reald)
					{
					case 0:
						if (mousex > headx)
							portal[1].ptd = 1;
						else
							portal[1].ptd = 5;
						break;
					case 1:
						if (k > 0)
						{
							if (mousex > headx)
								portal[1].ptd = 1;
							else
								portal[1].ptd = 5;
						}
						else
						{
							if (mousex > headx)
								portal[1].ptd = 2;
							else
								portal[1].ptd = 6;
						}
						break;
					case 2:
						if (mousex > headx)
							portal[1].ptd = 2;
						else
							portal[1].ptd = 6;
						break;
					case 3:
						if (k > 0)
						{
							if (mousex > headx)
								portal[1].ptd = 2;
							else
								portal[1].ptd = 6;
						}
						else
						{
							if (mousex > headx)
								portal[1].ptd = 1;
							else
								portal[1].ptd = 5;
						}
					}
					portal[1].ptx = mousex / blsize;
					portal[1].pty = mousey / blsize - 1;
				}
			}
			break;
		}
	}
	load_hitbox();//加载碰撞点，为下面改碰撞点做准备
	if (portal[0].ptd && portal[1].ptd)
	{
		int calc0 = portal[0].ptd - 1 >> 1;
		int calc1 = portal[1].ptd - 1 >> 1;//解算中间变量
		for (int i = 0; i < 6 - (calc0 & 1); i++)//蓝门
		{
			for (int j = 0; j < 5 + (calc0 & 1); j++)
			{
				int x = portal[0].ptx * 2 + (calc0 & 1) - (calc0 & 2) / 2 * 3 * (1 - (calc0 & 1)) + i;
				int y = portal[0].pty * 2 + 1 - (calc0 & 1) * ((calc0 & 2) / 2 * 3 + 1) + j;
				hitpoint[x][y] = hitpoint[trans(x * 10, y * 10, 0, 0) / 10][trans(x * 10, y * 10, 0, 1) / 10];//将另一扇门外的点映射至该门内，形式与角色的映射一样
			}
		}
		for (int i = 0; i < 6 - (calc1 & 1); i++)//橙门
		{
			for (int j = 0; j < 5 + (calc1 & 1); j++)
			{
				int x = portal[1].ptx * 2 + (calc1 & 1) - (calc1 & 2) / 2 * 3 * (1 - (calc1 & 1)) + i;
				int y = portal[1].pty * 2 + 1 - (calc1 & 1) * ((calc1 & 2) / 2 * 3 + 1) + j;
				hitpoint[x][y] = hitpoint[trans(x * 10, y * 10, 1, 0) / 10][trans(x * 10, y * 10, 1, 1) / 10];
			}
		}
		for (int i = 0; i < 2 + (calc0 & 1) * 3; i++)//挖空蓝门本身的碰撞点，下橙门同
		{
			for (int j = 0; j < 5 - (calc0 & 1) * 3; j++)
			{
				int x = portal[0].ptx * 2 + 1 - (calc0 == 0) + i;
				int y = portal[0].pty * 2 + 1 - (calc0 == 1) + j;
				hitpoint[x][y] = 0;
			}
		}
		for (int i = 0; i < 2 + (calc1 & 1) * 3; i++)//橙门
		{
			for (int j = 0; j < 5 - (calc1 & 1) * 3; j++)
			{
				int x = portal[1].ptx * 2 + 1 - (calc1 == 0) + i;
				int y = portal[1].pty * 2 + 1 - (calc1 == 1) + j;
				hitpoint[x][y] = 0;
			}
		}
	}
}
void map_load()//加载地图方块
{
	for (int i = 0; i < 50; i++)
		for (int j = 0; j < 30; j++)
		{
			if (block[i][j])//黑色底
			{
				setfillcolor(RGB(0, 0, 0));
				solidrectangle(blsize * i, blsize * j, blsize * i + blsize, blsize * j + blsize);
			}
			if (block[i][j] == 1)//可射门方块，白色
			{
				setfillcolor(RGB(255, 255, 255));
				solidrectangle(blsize * i + 1, blsize * j + 1, blsize * i + 19, blsize * j + 19);
			}
			if (block[i][j] == 2)//不可射门方块，灰色
			{
				setfillcolor(RGB(50, 50, 50));
				solidrectangle(blsize * i + 1, blsize * j + 1, blsize * i + 19, blsize * j + 19);
			}
		}
}
/*void scale()//绘制坐标轴，方便制图
{
	char s[10];
	setlinecolor(RGB(255, 255, 255));
	setlinestyle(PS_SOLID | PS_ENDCAP_FLAT, 1);
	for (int i = 0; i < 30; i++)
	{
		_stprintf_s(s, _T("%d"), i);
		outtextxy(0, i * 20, s);
		line(0, i * 20, 1000, i * 20);
	}
	for (int i = 0; i < 50; i++)
	{
		_stprintf_s(s, _T("%d"), i);
		outtextxy(i * 20, 0, s);
		line(i * 20, 0, i * 20, 600);
	}
}*/
//以下为一系列地图
void map0()
{
	cx = 320;
	cy = 360;
	mpr(2, 8, 6, 40, 24);
	mpr(0, 13, 10, 35, 20);
	mpr(1, 13, 9, 35, 9);
	mpr(1, 13, 21, 35, 21);
	mpr(1, 12, 10, 12, 20);
	mpr(1, 36, 10, 36, 20);
	endx = 580;
	endy = 360;
	endd = 0;
}
void map1()
{
	for (int i = 0; i < 50; i++)
		block[i][4] = 1;
	block[3][4] = 0;
	block[5][4] = 0;
	block[6][4] = 0;
	block[7][3] = 1;
	block[20][3] = 1;
	block[10][4] = 0;
	block[11][4] = 0;
	block[12][4] = 0;
	block[13][4] = 0;
	block[12][6] = 1;
	block[11][7] = 1;
	block[13][6] = 1;
	block[14][6] = 1;
	block[15][7] = 1;
	block[15][6] = 1;
	for (int i = 0; i < 30; i++)
		block[i][8] = 1;
	for (int i = 0; i < 50; i++)
		block[i][13] = 1;
	block[20][11] = 1;
	block[20][10] = 1;
	block[20][12] = 1;
}
void map2()
{
	cx = 200;
	cy = 200;
	for (int i = 4; i < 26; i++)
	{
		block[4][i] = 1;
		block[30][i] = 1;
	}
	for (int i = 4; i < 31; i++)
	{
		block[i][4] = 1;
		block[i][26] = 1;
	}
	for (int i = 6; i < 24; i++)
	{
		block[i][24] = 1;
	}
	mpr(1, 15, 20, 20, 20);
	mpr(1, 2, 2, 3, 28);
	mpr(1, 2, 2, 32, 4);
	mpr(1, 31, 4, 32, 28);
	mpr(1, 2, 26, 32, 28);
}
void map3()
{
	cx = 200;
	cy = 320;
	chd = 0;
	mpr(2, 5, 4, 40, 25);
	mpr(1, 8, 16, 8, 18);
	mpr(0, 9, 15, 15, 20);
	mpr(1, 12, 21, 14, 21);
	mpr(1, 18, 22, 18, 24);
	mpr(1, 12, 14, 14, 14);
	mpr(0, 16, 15, 19, 17);
	mpr(0, 19, 15, 23, 25);
	mpr(2, 21, 15, 21, 22);
	mpr(1, 24, 22, 24, 24);
	mpr(0, 22, 7, 23, 14);
	mpr(1, 24, 13, 24, 15);
	mpr(0, 16, 7, 31, 11);
	mpr(2, 25, 7, 25, 10);
	mpr(1, 17, 6, 19, 6);
	mpr(1, 17, 12, 19, 12);
	mpr(1, 32, 8, 32, 10);
	mpr(2, 9, 20, 10, 20);
	mpr(2, 9, 19, 9, 19);
	endx = 580;
	endy = 180;
	endd = 0;
}
void map4()
{
	cx = 240;
	cy = 320;
	chd = 0;
	mpr(2, 5, 4, 40, 25);
	mpr(1, 8, 16, 8, 18);
	mpr(0, 9, 15, 15, 20);
	mpr(1, 9, 21, 11, 21);
	mpr(1, 18, 22, 18, 24);
	mpr(1, 12, 14, 14, 14);
	mpr(0, 16, 15, 19, 17);
	mpr(0, 19, 15, 24, 25);
	mpr(2, 20, 15, 20, 21);
	mpr(2, 20, 23, 20, 25);
	mpr(0, 23, 7, 24, 14);
	mpr(1, 25, 13, 25, 15);
	mpr(0, 16, 7, 31, 11);
	mpr(2, 25, 7, 25, 10);
	mpr(1, 20, 6, 22, 6);
	mpr(1, 15, 8, 15, 10);
	mpr(1, 32, 8, 32, 10);
	mpr(2, 23, 19, 23, 23);
	mpr(2, 23, 13, 23, 14);
	endx = 580;
	endy = 180;
	endd = 0;
}
void map5()
{
	cx = 300;
	cy = 340;
	mpr(2, 8, 6, 40, 24);
	mpr(0, 13, 10, 35, 20);
	mpr(2, 19, 18, 19, 20);
	mpr(2, 20, 18, 20, 20);
	mpr(2, 21, 19, 21, 20);
	mpr(2, 22, 20, 22, 20);
	mpr(2, 24, 10, 26, 14);
	mpr(1, 24, 11, 24, 13);
	mpr(1, 12, 18, 12, 20);
	mpr(2, 31, 18, 31, 20);
	mpr(1, 36, 18, 36, 20);
	endx = 660;
	endy = 360;
	endd = 0;
}
void map6()
{
	cx = 300;
	cy = 340;
	mpr(2, 8, 6, 40, 24);
	mpr(0, 13, 10, 35, 20);
	mpr(2, 19, 15, 23, 20);
	mpr(2, 14, 15, 31, 15);
	mpr(2, 29, 10, 29, 13);
	mpr(2, 28, 10, 28, 12);
	mpr(2, 27, 10, 27, 11);
	mpr(2, 26, 10, 26, 10);
	mpr(1, 12, 11, 12, 13);
	mpr(1, 12, 16, 12, 20);
	mpr(1, 20, 15, 22, 15);
	mpr(1, 23, 17, 23, 20);
	mpr(1, 24, 21, 33, 21);
	endx = 560;
	endy = 360;
	endd = 0;
}
void map7()
{
	cx = 300;
	cy = 340;
	mpr(2, 8, 6, 40, 24);
	mpr(0, 13, 10, 35, 20);
	mpr(2, 20, 14, 22, 20);
	mpr(2, 23, 14, 31, 16);
	mpr(1, 15, 9, 17, 9);
	mpr(1, 15, 21, 17, 21);
	mpr(1, 12, 10, 12, 12);
	mpr(1, 32, 21, 34, 21);
	mpr(1, 22, 18, 22, 20);
	endx = 580;
	endy = 360;
	endd = 0;
}
void map8()
{
	cx = 340;
	cy = 340;
	mpr(2, 8, 6, 40, 24);
	mpr(0, 13, 10, 35, 20);
	mpr(2, 15, 14, 31, 16);
	mpr(2, 22, 11, 24, 20);
	mpr(1, 18, 21, 20, 21);
	mpr(1, 26, 21, 28, 21);
	mpr(1, 12, 13, 12, 15);
	mpr(1, 36, 13, 36, 15);
	mpr(1, 22, 9, 24, 9);
	endx = 580;
	endy = 360;
	endd = 0;
}
void map9()
{
	cx = 320;
	cy = 360;
	mpr(2, 8, 6, 40, 24);
	mpr(0, 13, 10, 35, 20);
	mpr(2, 14, 15, 27, 17);
	mpr(2, 19, 20, 19, 20);
	mpr(2, 20, 19, 20, 20);
	mpr(2, 21, 18, 27, 20);
	mpr(2, 28, 15, 31, 16);
	mpr(2, 23, 10, 25, 13);
	mpr(1, 15, 17, 19, 17);
	mpr(1, 12, 15, 12, 17);
	mpr(1, 16, 9, 18, 9);
	mpr(1, 36, 13, 36, 15);
	mpr(1, 31, 21, 33, 21);
	endx = 580;
	endy = 360;
	endd = 0;
}
void map10()
{
	cx = 360;
	cy = 380;
	mpr(2, 8, 6, 40, 24);
	mpr(0, 12, 9, 36, 21);
	mpr(2, 15, 14, 15, 20);
	mpr(2, 13, 21, 13, 21);
	mpr(2, 16, 14, 32, 16);
	mpr(2, 23, 17, 25, 21);
	mpr(2, 32, 12, 32, 13);
	//	mpr(2, 29, 13, 29, 13);//引出蹭墙跳，还是要面对的！
	mpr(1, 11, 14, 11, 16);
	mpr(1, 17, 8, 19, 8);
	mpr(1, 17, 16, 19, 16);
	mpr(1, 23, 19, 23, 21);
	mpr(1, 31, 22, 33, 22);
	mpr(1, 37, 14, 37, 16);
	endx = 560;
	endy = 380;
	endd = 0;
}
void map11()
{
	cx = 320;
	cy = 360;
	mpr(2, 8, 6, 40, 24);
	mpr(0, 13, 10, 35, 20);
	mpr(2, 13, 10, 20, 15);
	mpr(2, 23, 18, 23, 20);
	mpr(2, 26, 10, 28, 19);
	mpr(1, 14, 15, 19, 15);
	mpr(1, 12, 18, 12, 20);
	mpr(1, 36, 18, 36, 20);
	mpr(1, 26, 11, 26, 18);
	endx = 620;
	endy = 360;
	endd = 0;
}
void map12()
{
	cx = 320;
	cy = 360;
	mpr(2, 8, 6, 40, 24);
	mpr(0, 13, 10, 35, 20);
	mpr(2, 14, 14, 25, 17);
	mpr(2, 18, 20, 20, 20);
	mpr(2, 26, 10, 28, 17);
	mpr(2, 29, 10, 35, 15);
	mpr(2, 31, 19, 31, 20);
	mpr(2, 32, 18, 32, 20);
	mpr(1, 12, 14, 12, 17);
	mpr(1, 17, 9, 19, 9);
	mpr(1, 26, 11, 26, 13);
	mpr(1, 17, 17, 21, 17);
	mpr(1, 36, 18, 36, 20);
	endx = 680;
	endy = 360;
	endd = 0;
}
void map13()
{
	cx = 260;
	cy = 380;
	mpr(2, 8, 6, 40, 24);
	mpr(0, 12, 9, 36, 21);
	mpr(2, 22, 13, 36, 15);
	mpr(2, 18, 9, 20, 17);
	mpr(2, 15, 18, 15, 21);
	mpr(2, 22, 16, 28, 18);
	mpr(2, 31, 20, 31, 21);
	mpr(2, 32, 19, 32, 21);
	mpr(2, 34, 16, 34, 18);
	mpr(2, 17, 25, 26, 28);
	mpr(0, 23, 13, 27, 17);
	mpr(0, 18, 22, 20, 27);
	mpr(0, 21, 25, 23, 27);
	mpr(2, 22, 27, 22, 27);
	mpr(2, 30, 21, 30, 21);
	mpr(1, 11, 19, 11, 21);
	mpr(1, 18, 11, 18, 13);
	mpr(1, 20, 13, 20, 15);
	mpr(1, 24, 22, 26, 22);
	mpr(1, 37, 19, 37, 21);
	mpr(1, 24, 8, 26, 8);
	mpr(1, 30, 13, 32, 13);
	mpr(1, 24, 25, 24, 27);
	endx = 680;
	endy = 200;
	endd = 0;
}
void map14() {
	cx = 300;
	cy = 340;
	mpr(2, 8, 6, 40, 24);
	mpr(0, 13, 10, 35, 20);
	mpr(2, 17, 13, 33, 16);
	mpr(2, 33, 12, 33, 12);
	mpr(2, 20, 17, 26, 20);
	mpr(1, 16, 9, 18, 9);
	mpr(1, 12, 17, 12, 19);
	mpr(1, 36, 18, 36, 20);
	endx = 580;
	endy = 360;
	endd = 0;
}
void map15() {
	cx = 580;
	cy = 320;
	mpr(2, 8, 6, 40, 24);
	mpr(0, 13, 11, 35, 20);
	mpr(2, 14, 19, 34, 19);
	mpr(2, 20, 13, 20, 13);
	mpr(2, 21, 12, 21, 15);
	mpr(2, 25, 11, 25, 18);
	mpr(2, 26, 13, 26, 13);
	mpr(1, 20, 10, 22, 10);
	mpr(1, 12, 18, 12, 20);
	mpr(1, 36, 18, 36, 20);
	mpr(1, 26, 10, 28, 10);
	endx = 360;
	endy = 320;
	endd = 0;
}
void scale()//绘制坐标轴，方便制图
{
	TCHAR s[10];
	setlinecolor(RGB(255, 255, 255));
	setlinestyle(PS_SOLID | PS_ENDCAP_FLAT, 1);
	for (int i = 0; i < 30; i++)
	{
		_stprintf_s(s, _T("%d"), i);
		outtextxy(0, i * 20, s);
		line(0, i * 20, 1000, i * 20);
	}
	for (int i = 0; i < 50; i++)
	{
		_stprintf_s(s, _T("%d"), i);
		outtextxy(i * 20, 0, s);
		line(i * 20, 0, i * 20, 600);
	}
}
int play_map(int map)
{
	d_flag = 0;//伏笔回收
start://这里重开
	level_reset();
	switch (map)
	{
	case 0:map0(); break;
	case 1:map1(); break;
	case 2:map2(); break;
	case 3:map3(); break;
	case 4:map4(); break;
	case 5:map5(); break;
	case 6:map6(); break;
	case 7:map7(); break;
	case 8:map8(); break;
	case 9:map9(); break;
	case 10:map10(); break;
	case 11:map11(); break;
	case 12:map12(); break;
	case 13:map13(); break;
	case 14:map14(); break;
	case 15:map15(); break;
	}
	while (1)//引入所有游戏相关函数
	{
		if (GetAsyncKeyState(VK_LCONTROL) && portal[1].ptd)break;//跳关
		if (GetAsyncKeyState('S') && cx - endx<10 && cx - endx>-10 && cy - endy<10 && cy - endy>-10 && chd == endd)
			break;//按下键过关
		if (GetAsyncKeyState('R'))
			goto start;//按R重开
		BeginBatchDraw();
		cleardevice();
		if (0)//绘制坐标轴
		{
			scale();///////
		}
		setfillcolor(RGB(200, 200, 200));
		solidrectangle(endx, endy, endx + chsize[endd & 1], endy + chsize[1 - endd & 1]);//绘制过关的门
		for (int i = 0; i < 50; i++)
			for (int j = 0; j < 30; j++)
				if (block[i][j] < 0)
					block[i][j] = 1;//覆盖消失的门
		setfillcolor(RGB(230, 230, 125));//角色及投影绘制
		if (can_draw)
			solidrectangle(px, py, px + chsize[pd & 1], py + chsize[1 - pd & 1]);
		can_draw = 0;
		solidrectangle(cx, cy, cx + chsize[chd & 1], cy + chsize[1 - chd & 1]);
		map_load();
		if (0)
			for (int i = 0; i < 100; i++)
				for (int j = 0; j < 60; j++)
					if (hitpoint[i][j] && hitpoint[i][j] != 1)
					{
						setfillcolor(RGB(0, 255, 0));
						solidrectangle(i * 10, j * 10, i * 10 + 1, j * 10 + 1);
					}//显示地图碰撞点以供调试
		drawreset();
		reload_position();
		portal_use();
		move();
		FlushBatchDraw();
		Sleep(20);//停顿
	}
	return 1;
}
int main()
{
	initgraph(1000, 600);
	play_map(0);//概念 concept
	play_map(5);//高山 mountain
	play_map(6);//旋转 spin
	play_map(7);//远跳 leap
	play_map(8);//大脑 brain*
	play_map(9);//穿梭 shuttle
	play_map(10);//支点 fulcrum*
	play_map(3);//裂缝 gap*
	play_map(14);
	play_map(11);//倒立 upend
	play_map(12);//远见 foresight*
	play_map(15);
	play_map(13);//阀门 valve
	play_map(4);//深渊 abyss*
	//下方关卡会使用用游戏代码“特性”，暂且未加入
	//破壁 breakthrough
	//废墟 ruins
	//重生 revive
	///熵增 entropy
}