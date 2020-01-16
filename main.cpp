#define GLEW_STATIC
#include <gl/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glfw/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <vector>
#include<iostream>

struct MapCube;
struct Player;

//窗口大小回调
void resize(GLFWwindow *, int w, int h);
//键盘响应
void keyboard(GLFWwindow *, int key, int scancode, int action, int mod);
//渲染
void render();
//渲染方块
void renderCube(MapCube cube);
//加载地图
void readMap(char (*map)[9], int col, int row);
//更新逻辑
void update();
//物理碰撞检测
void physicsUpdate();
//角色和立方体碰撞
void collisionPlayerBox(Player &player, MapCube &cube);
//初始化
void initialize();
//创建shader程序
GLuint createShaderProgram();
//加载纹理
GLuint loadTexture(const char *filePath);

//窗口
GLFWwindow *window = 0;
//窗口大小
float width = 1280, height = 720;
//角色速度
float playerSpeed = 0.5;
//箱子速度
float crateSpeed = playerSpeed*2;

//立方体vbo vao
GLuint cubeVBO, cubeVAO;
//shader
GLuint shaderProgram;
//纹理
GLuint textures[4];

//玩家数据
struct Player
{
	static const float sightHeihgt;	        //视线高度
	static const float collisionDistance;	//角色半径
	glm::vec3 pos;	//位置
	float lookYaw;	//观察角度   左右转头，左右晃动鼠标
	float lookPitch;//观察角度   上下转头，上下晃动鼠标

	//得到观察矩阵
	glm::mat4 getViewMatrix()
	{
		pos.y = sightHeihgt;
		return glm::lookAt(pos, lookDir() + pos, glm::vec3(0, 1, 0));//eye center up lookAt为glm库自带的函数
	}
	   /* 第一组eyex, eyey, eyez 相机在世界坐标的位置
		第二组centerx, centery, centerz 相机镜头对准的物体在世界坐标的位置
		第三组upx, upy, upz 相机向上的方向在世界坐标中的方向*/


	//得到观察方向
	glm::vec3 lookDir()
	{
		float sp = glm::sin(glm::radians(lookPitch)), cp = glm::cos(glm::radians(lookPitch));//glm::radians()	角度制转弧度制
		float sy = glm::sin(glm::radians(lookYaw)), cy = glm::cos(glm::radians(lookYaw));
		return glm::vec3(cp*sy, sp, cp*cy);
	}
} player;

const float Player::sightHeihgt = 1.2f;
const float Player::collisionDistance = 0.6f;

//方块类型枚举
enum CubeType
{
	Wall=0,	        //墙
	Crate=1,		//箱子
	Ceiling=2,	//天花板
	Floor=3,		//地板
	DestSpot,	//箱子目标点
};

//地图方块
struct MapCube
{
	glm::vec3 pos;	//位置
	CubeType type;		//方块类型
	bool inPosition;	//是否已经移动到位置(只对type==Crate有意义) 只有箱子才可以移动

	unsigned char canMoveDir;		//可以移动方向 0 1 2 3  ----> +x -x +z -z
};

//滑动动画数据
struct CubeSlide
{
	MapCube *cube;	//滑动cube
	glm::vec3 slideDir;	//滑动方向
	glm::vec3 destPos;	//终点位置
};

//处理物理检测的cube
std::vector<MapCube> mapCubes; //vector是表示可以改变大小的数组的序列容器。
//其他辅助cube
std::vector<MapCube> otherCubes;

//其他辅助cube
std::vector<CubeSlide> cubeSlideVec;

void main()
{
	glfwInit();  // /初始化GLFW
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);//告诉GLFW我们使用的OpenGL版本是3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	
	window = glfwCreateWindow(width, height, "OpenGL", 0, 0);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);//隐藏鼠标
	glfwSetKeyCallback(window, keyboard);
	glfwMakeContextCurrent(window);////将我们窗口的上下文设置为当前线程的主上下文
	if (glewInit()!=GLEW_OK)
	{
		return;
	}

	initialize();

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();  // 检查有没有触发什么事件
		update();
		render();
		glfwSwapBuffers(window);
	}
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	glfwDestroyWindow(window);

	glfwTerminate();//释放/删除之前的分配的所有资源
}

void resize(GLFWwindow *, int w, int h)
{
	if (!w||!h)
	{
		return;
	}
	width = w;
	height = h;
	glViewport(0, 0, w, h);
}

void keyboard(GLFWwindow * window, int key, int scancode, int action, int mod)
{
	if (key==256&&action==GLFW_RELEASE)
	{
		glfwSetWindowShouldClose(window, 1);
	}
}

void render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(shaderProgram);
	int VLoc = glGetUniformLocation(shaderProgram, "V");
	int PLoc = glGetUniformLocation(shaderProgram, "P");
	int tintColorLoc = glGetUniformLocation(shaderProgram, "tintColor");
	int lightIntensityLoc = glGetUniformLocation(shaderProgram, "lightIntensity");

	glUniform3f(tintColorLoc, 1, 1, 1);
	glUniform3f(lightIntensityLoc, 1, 1, 1);

	glUniformMatrix4fv(VLoc, 1, GL_FALSE, glm::value_ptr(player.getViewMatrix()));
	glUniformMatrix4fv(PLoc, 1, GL_FALSE, glm::value_ptr(glm::perspective(glm::radians(45.0f),width/height,0.001f,100.0f)));

	for (int i = 0; i < mapCubes.size(); ++i)
	{
		renderCube(mapCubes[i]);
	}
	for (int i = 0; i < otherCubes.size(); ++i)
	{
		renderCube(otherCubes[i]);
	}
}

void renderCube(MapCube cube)
{
	int texLoc = glGetUniformLocation(shaderProgram, "tex");
	int uvScaleLoc = glGetUniformLocation(shaderProgram, "uvScale");
	int MLoc = glGetUniformLocation(shaderProgram, "M");

	if (cube.type != DestSpot)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textures[cube.type]);
		glUniform1i(texLoc, 0);

		glUniform1f(uvScaleLoc, cube.type == Ceiling ? 2 : 1);
		glUniformMatrix4fv(MLoc, 1, GL_FALSE, glm::value_ptr(glm::translate(glm::mat4(1), cube.pos)));

		glBindVertexArray(cubeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);
	}
}

void readMap(char(*map)[9], int col, int row)
{
	for (int j=0;j<row;++j)
	{
		for (int i=0;i<col;++i)
		{
			switch (map[j][i])
			{
			case 'W':		//墙
			{
				mapCubes.emplace_back(MapCube{ glm::vec3(i,0,j),Wall,false });//位置 类型 是否已经移动到位置(只对type==Crate有
				otherCubes.emplace_back(MapCube{ glm::vec3(i,1,j),Wall,false });
			}
				break;
			case 'D':		//目标点
			{
				otherCubes.emplace_back(MapCube{ glm::vec3(i,0,j),DestSpot,false });
			}
				break;
			case 'B':		//箱子
			{
				mapCubes.emplace_back(MapCube{ glm::vec3(i,0,j),Crate,false });
			}
				break;
			case 'X':		//箱子和目标点
			{
				mapCubes.emplace_back(MapCube{ glm::vec3(i,0,j),Crate,false });
				otherCubes.emplace_back(MapCube{ glm::vec3(i,0,j),DestSpot,false });
			}
				break;
			case 'P':		//玩家
				player.pos=glm::vec3(i,0,j);
				break;
			default:
				break;
			}
			otherCubes.emplace_back(MapCube{ glm::vec3(i,-1,j),Floor,false });
			otherCubes.emplace_back(MapCube{ glm::vec3(i,2,j),Ceiling,false });
		}
	}
}

void update()
{
	
/*------实现平滑移动-------
根据处理器的能力不同，有的人在同一段时间内会比其他人绘制更多帧。每个人的运动速度就都不同了。当你要发布的你应用的时候，你必须确保在所有硬件上移动速度都一样。
图形和游戏应用通常有回跟踪一个deltaTime变量，它储存渲染上一帧所用的时间。我们把所有速度都去乘以deltaTime值。当我们的deltaTime变大时意味着上一帧渲染花了更多时间，
所以这一帧使用这个更大的deltaTime的值乘以速度，会获得更高的速度，这样就与上一帧平衡了。（当deltaTime变小时同理。）使用这种方法时，无论机器快还是慢，速度都会保持一致，这样每个用户的体验就都一样了。
*/
	double currentFrame = glfwGetTime();//读取时间  要创建平滑的动画，需要时间源，GLFW提供一个计时器，该计时器返回自初始化以来的秒数。
	static double lastFrame;
	float deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;


/*--------FPS风格的摄像机鼠标输入---------
计算鼠标和上一帧的偏移量。
把偏移量添加到摄像机和俯仰角和偏航角中。
对偏航角和俯仰角进行最大和最小值的限制。
*/
	static double lastX, lastY;
	double currentX, currentY;
    glfwGetCursorPos(window, &currentX, &currentY);//将鼠标的实时位置给currentX currentY
    float offsetX = currentX - lastX, offsetY = currentY - lastY;//计算鼠标和上一帧的偏移量。
	
    player.lookYaw -= offsetX*0.1;//把偏移量添加到摄像机和俯仰角和偏航角中。
    player.lookPitch -= offsetY*0.1;

	player.lookPitch = glm::clamp(player.lookPitch, -89.9f, 89.9f);//把player.lookPitch限定在 -89.9f, 89.9f之间

	lastX = currentX;
    lastY = currentY;
	

	glm::vec3 lookDir = player.lookDir();
	lookDir.y = 0;
	glm::vec3 forward = glm::normalize(lookDir);
	glm::vec3 right = glm::cross(forward, glm::vec3(0, 1, 0)); 

	if (glfwGetKey(window, 'A'))
	{
		player.pos -= right*deltaTime*playerSpeed;
	}

	if (glfwGetKey(window, 'D'))
	{
		player.pos += right*deltaTime*playerSpeed;
	}

	if (glfwGetKey(window, 'W'))
	{
		player.pos += forward*deltaTime*playerSpeed;
	}

	if (glfwGetKey(window, 'S'))
	{
		player.pos -= forward*deltaTime*playerSpeed;
	}
	physicsUpdate();

	//滑动箱子
	for (auto iter = cubeSlideVec.begin(); iter != cubeSlideVec.end();)
	{
		iter->cube->pos = iter->cube->pos + iter->slideDir*crateSpeed*deltaTime;
		if (glm::dot(iter->cube->pos-iter->destPos,iter->slideDir)>=0.0f)//终点位置和滑动方向相同
		{
			iter->cube->pos = iter->destPos;
			iter = cubeSlideVec.erase(iter);
		}
		else
		{
			iter++;
		}
	}
}

void physicsUpdate()
{
	//更新箱子可移动状态
	for (int i=0;i<mapCubes.size();++i)
	{
		if (mapCubes[i].type!=Crate) continue;//只有箱子才可以移动

		mapCubes[i].canMoveDir = 0xff;

		for (int j=0;j<mapCubes.size();++j)
		{
			glm::vec3 dis = mapCubes[i].pos - mapCubes[j].pos;
			if (length(dis)<=1.0001)// 如果箱子间的距离过小 则那个方向不能推动
			{
				/*
				    ~1  1 << 0  1   右
					~2  1 << 1  2   左
					~3  1 << 2  4   下
					~4  1 << 3  8   上
				*/
				if (dis.x>0.5) mapCubes[i].canMoveDir &= ~1;       
				if (dis.x<-0.5) mapCubes[i].canMoveDir &= ~2;
				if (dis.z > 0.5) mapCubes[i].canMoveDir &= ~4;
				if (dis.z < -0.5) mapCubes[i].canMoveDir &= ~8;
			}
		}
	}

	//碰撞和更新角色位置
	for(int i=0;i<mapCubes.size();++i)
	{
		MapCube &cube=mapCubes[i];
		collisionPlayerBox(player, cube);
	}
}

void collisionPlayerBox(Player & player, MapCube & cube)
{
	glm::vec3 dis = player.pos - cube.pos;
	glm::vec3 slideDir = glm::vec3(0);
	bool boxSlide = false;
	if (glm::abs(dis.x) < Player::collisionDistance && glm::abs(dis.z) < Player::collisionDistance)//collisionDistance  0.6  两个方向均小于最小碰撞距离
	{
		if (glm::abs(dis.x) > glm::abs(dis.z))
		{
			float r = glm::abs(dis.z) / glm::abs(dis.x);
			if (dis.x > 0)                                               //x大于0 右
			{
				boxSlide = cube.type == Crate && (cube.canMoveDir & 1) && r<0.5;
				slideDir.x = -1;
				dis.x = Player::collisionDistance;
			}
			else
			{
				boxSlide = cube.type == Crate && (cube.canMoveDir & 2) && r<0.5;
				slideDir.x = 1;
				dis.x = -Player::collisionDistance;
			}
		}
		else
		{
			float r = glm::abs(dis.x) / glm::abs(dis.z);
			if (dis.z > 0)
			{
				boxSlide = cube.type == Crate && (cube.canMoveDir & 4) && r<0.5;
				slideDir.z = -1;
				dis.z = Player::collisionDistance;
			}
			else
			{
				boxSlide = cube.type == Crate && (cube.canMoveDir & 8) && r<0.5;
				slideDir.z = 1;
				dis.z = -Player::collisionDistance;
			}
		}
		if (boxSlide)
		{
			bool alreadySliding = false;
			for(int i=0;i<cubeSlideVec.size();++i)
			{
				if (cubeSlideVec[i].cube==&cube)
				{
					alreadySliding = true;
				}
			}
			if (!alreadySliding)
			{
				cubeSlideVec.push_back({ &cube,slideDir,cube.pos + slideDir });
			}
		}
		else
		{
			player.pos = cube.pos + dis;
		}
	}
}

void initialize()
{
	char map[][9] = {
		"  WWWWW ",
		"WWW   W ",
		"WDPB  W ",
		"WWW BDW ",
		"WDWWB W ",
		"W W D WW",
		"WB XBBDW",
		"W   D  W",
		"WWWWWWWW",
	};
	readMap(map, std::size(map), std::size(map[0]));

	shaderProgram = createShaderProgram();
	textures[Wall] = loadTexture("assets//wall.jpg");
	textures[Crate] = loadTexture("assets//crate.jpg");
	textures[Floor] = loadTexture("assets//floor.jpg");
	textures[Ceiling] = loadTexture("assets//ceiling.jpg");

	float boxData[] = {
		-0.500000,1.000000,0.500000,-1.0000,0.0000,0.0000,1.000000,0.000000,
		-0.500000,0.000000,-0.500000,-1.0000,0.0000,0.0000,0.000000,1.000000,
		-0.500000,0.000000,0.500000,-1.0000,0.0000,0.0000,0.000000,0.000000,
		-0.500000,1.000000,-0.500000,0.0000,0.0000,-1.0000,1.000000,0.000000,
		0.500000,0.000000,-0.500000,0.0000,0.0000,-1.0000,0.000000,1.000000,
		-0.500000,0.000000,-0.500000,0.0000,0.0000,-1.0000,0.000000,0.000000,
		0.500000,1.000000,-0.500000,1.0000,0.0000,0.0000,1.000000,0.000000,
		0.500000,0.000000,0.500000,1.0000,0.0000,0.0000,0.000000,1.000000,
		0.500000,0.000000,-0.500000,1.0000,0.0000,0.0000,0.000000,0.000000,
		0.500000,1.000000,0.500000,0.0000,0.0000,1.0000,1.000000,0.000000,
		-0.500000,0.000000,0.500000,0.0000,0.0000,1.0000,0.000000,1.000000,
		0.500000,0.000000,0.500000,0.0000,0.0000,1.0000,0.000000,0.000000,
		0.500000,0.000000,-0.500000,0.0000,-1.0000,0.0000,1.000000,0.000000,
		-0.500000,0.000000,0.500000,0.0000,-1.0000,0.0000,0.000000,1.000000,
		-0.500000,0.000000,-0.500000,0.0000,-1.0000,0.0000,0.000000,0.000000,
		-0.500000,1.000000,-0.500000,0.0000,1.0000,0.0000,1.000000,0.000000,
		0.500000,1.000000,0.500000,0.0000,1.0000,0.0000,0.000000,1.000000,
		0.500000,1.000000,-0.500000,0.0000,1.0000,0.0000,0.000000,0.000000,
		-0.500000,1.000000,0.500000,-1.0000,0.0000,0.0000,1.000000,0.000000,
		-0.500000,1.000000,-0.500000,-1.0000,0.0000,0.0000,1.000000,1.000000,
		-0.500000,0.000000,-0.500000,-1.0000,0.0000,0.0000,0.000000,1.000000,
		-0.500000,1.000000,-0.500000,0.0000,0.0000,-1.0000,1.000000,0.000000,
		0.500000,1.000000,-0.500000,0.0000,0.0000,-1.0000,1.000000,1.000000,
		0.500000,0.000000,-0.500000,0.0000,0.0000,-1.0000,0.000000,1.000000,
		0.500000,1.000000,-0.500000,1.0000,0.0000,0.0000,1.000000,0.000000,
		0.500000,1.000000,0.500000,1.0000,0.0000,0.0000,1.000000,1.000000,
		0.500000,0.000000,0.500000,1.0000,0.0000,0.0000,0.000000,1.000000,
		0.500000,1.000000,0.500000,0.0000,0.0000,1.0000,1.000000,0.000000,
		-0.500000,1.000000,0.500000,0.0000,0.0000,1.0000,1.000000,1.000000,
		-0.500000,0.000000,0.500000,0.0000,0.0000,1.0000,0.000000,1.000000,
		0.500000,0.000000,-0.500000,0.0000,-1.0000,0.0000,1.000000,0.000000,
		0.500000,0.000000,0.500000,0.0000,-1.0000,0.0000,1.000000,1.000000,
		-0.500000,0.000000,0.500000,0.0000,-1.0000,0.0000,0.000000,1.000000,
		-0.500000,1.000000,-0.500000,0.0000,1.0000,0.0000,1.000000,0.000000,
		-0.500000,1.000000,0.500000,0.0000,1.0000,0.0000,1.000000,1.000000,
		0.500000,1.000000,0.500000,0.0000,1.0000,0.0000,0.000000,1.000000,
	};

	glGenVertexArrays(1, &cubeVAO);//两个参数，第一个为需要创建的缓存数量。第二个为用于存储单一ID或多个ID的GLuint变量或数组的地址
	glBindVertexArray(cubeVAO);

	glGenBuffers(1, &cubeVBO);//创建缓冲对象
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);//激活缓冲区对象

    glBufferData(GL_ARRAY_BUFFER, sizeof(boxData), boxData, GL_STATIC_DRAW);//用数据分配和初始化缓冲区对象
	glEnableVertexAttribArray(0);//允许顶点着色器读取GPU（服务器端）数据。

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);//使用该函数来给着色器中的in类型的属性变量传递数据
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3*sizeof(float)));
	glEnableVertexAttribArray(2);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
	glBindBuffer(GL_ARRAY_BUFFER,0);
	glBindVertexArray(0);

	glEnable(GL_DEPTH_TEST);
}

GLuint createShaderProgram()
{
	const char *vertSrc = R"glsl(
#version 330
layout (location=0) in vec4 position;
layout (location=1) in vec3 normal;
layout (location=2) in vec2 texCoord;

out VS_OUT
{
	vec3 normal;
	vec2 texCoord;
	vec3 fragPos;
} vs_out;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform float uvScale;
void main()
{
	gl_Position=P*(V*(M*position));
	vs_out.normal=normal*inverse(mat3(V*M));
	vs_out.fragPos=(V*(M*position)).xyz;
	vs_out.texCoord=texCoord.yx*uvScale;
}
)glsl";


	const char *fragSrc = R"glsl(
#version 330
in VS_OUT
{
	vec3 normal;
	vec2 texCoord;
	vec3 fragPos;
} fs_in;

uniform sampler2D tex;
uniform vec3 tintColor;
uniform vec3 lightIntensity;

out vec4 color;

void main()
{
	float attanuation=1/(dot(fs_in.fragPos,fs_in.fragPos)+10)*10;
	vec3 L=normalize(-fs_in.fragPos);
	vec3 N=normalize(fs_in.normal);
	vec3 E=L;
	vec3 H=normalize(L+E);

	float diff=max(0.0,dot(L,N));
	float spec=pow(max(0.0,dot(H,N)),64.0);

	vec3 baseColor=texture(tex,fs_in.texCoord).rgb*tintColor;
	color=vec4(0,0,0,1);
	color.rgb=(0.3+diff*0.7)*baseColor*lightIntensity*attanuation;
	//color.rgb=(0.1+diff)*baseColor*lightIntensity;
}

)glsl";

	GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
	GLuint program = glCreateProgram();

	glShaderSource(vertShader, 1, &vertSrc, 0);
	glCompileShader(vertShader);
	glShaderSource(fragShader, 1, &fragSrc, 0);
	glCompileShader(fragShader);

	glAttachShader(program, vertShader);
	glAttachShader(program, fragShader);

	glLinkProgram(program);

	glDetachShader(program, vertShader);
	glDetachShader(program, fragShader);

	glDeleteShader(vertShader);
	glDeleteShader(fragShader);

	int logLen = 0;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLen);
	if (logLen)
	{
		char *info = new char[logLen + 1];
		info[logLen] = 0;
		glGetProgramInfoLog(program, logLen, 0, info);
		printf("Program Error:\n%s", info);
		delete[] info;
	}

	return program;
}

GLuint loadTexture(const char * filePath)
{
	GLuint tex = 0;
	int w, h;
	unsigned char *data = stbi_load(filePath, &w, &h, 0, 4);
	if (data)
	{
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
		free(data);
	}
	return tex;
}
