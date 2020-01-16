## Question1：How does the user interact with your executable? How do you open and control the software you wrote (exe file)?

Press and hold WSAD to move the perspective and slide the mouse to make the camera roam in the scene. When approaching the box from the front of the box, push the box to move in the scene.

Press ESC to exit the game

Just click the Sokoban.exe file in the Sokoban \ directory to run.

## Question2：How does the program code work? How do the classes and functions fit together and who does what?

## 1．Achieve smooth camera and player movement

### 1.1Implementation of smooth movement

Depending on the capabilities of the processor, some people will draw more frames than others during the same period of time. Everyone's speed is different. When you are releasing your application, you must ensure that the speed of movement is the same on all hardware.

Graphics and game applications usually have a deltaTime variable that stores the time it took to render the previous frame. We multiply all speeds by the deltaTime value. When our deltaTime becomes larger, it means that the previous frame took more time to render, so this frame uses this larger deltaTime value multiplied by the speed to get a higher speed, so that it is balanced with the previous frame . (The same applies when the deltaTime becomes smaller.) When using this method, the speed will be the same whether the machine is fast or slow, so that the experience of each user is the same.

`	double currentFrame = glfwGetTime();//读取时间  要创建平滑的动画，需要时间源，GLFW提供一个计时器，该计时器返回自初始化以来的秒数
	static double lastFrame;
	float deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;`

### 1.2Base direction vector of character movement

 Defines two basic direction vectors forward, right for people's movement; used for calculation of direction when moving;

`	glm::vec3 forward = glm::normalize(lookDir);
	glm::vec3 right = glm::cross(forward, glm::vec3(0, 1, 0)); `

### 1.3Update of the coordinates of the character in the world coordinate system

Here WSAD represents the up, down, left, and right movements respectively. When a certain direction key is pressed, the position of the character in the world coordinate system is constantly changed and updated to achieve the movement effect.

Position of the character in the world coordinate system + = direction vector * deltaTime * moving speed

Take the character moving forward (W) as an example:

`if (glfwGetKey(window, 'W'))
	{
		player.pos += forward*deltaTime*playerSpeed;
	}`

## 2．Camera free viewing with mouse

Euler angles are three values that can represent any rotation in 3D space. There are three types: pitch, yaw, and roll.

![](C:\Users\Andrew\Desktop\typora pic\1.png)

### 2.1The implementation of smooth movement of the mouse angle of view (in the update () function, each frame is continuously updated)

##### 2.1.1Calculate the offset between the mouse and the previous frame:

`	static double lastX, lastY;
	double currentX, currentY;
	glfwGetCursorPos(window, &currentX, &currentY);//将鼠标的实时位置给currentX currentY
	float offsetX = currentX - lastX, offsetY = currentY - lastY;//计算鼠标和上一帧的偏移量。
	player.lookYaw -= offsetX * 0.1;//把偏移量添加到摄像机和俯仰角和偏航角中。
	player.lookPitch -= offsetY * 0.1;
	lastX = currentX;
	lastY = currentY;`

### 2.1.2Add offset to camera and Pitch and Yaw corners

`    player.lookYaw -= offsetX*0.1;//把偏移量添加到摄像机和俯仰角和偏航角中。
     player.lookPitch -= offsetY*0.1;`

##### 2.1.2.1Conversion of angle

![](C:\Users\Andrew\Desktop\typora pic\2.png)

Formula for length in x and y directions: define the length of the hypotenuse as 1

Then the length of the adjacent side iscos x/h = cos x/1 = cos x

The length of the opposite side is     sin y/h = sin y/1 = sin y

![](C:\Users\Andrew\Desktop\typora pic\3.png)

From the figure we can see that the y value of a given Pitch angle is equal to sinθ (the Yaw angle is similar), so bring it into the calculation:

`	glm::vec3 lookDir()
	{
		float sp = glm::sin(glm::radians(lookPitch)), cp = glm::cos(glm::radians(lookPitch));//glm::radians()	角度制转弧度制
		float sy = glm::sin(glm::radians(lookYaw)), cy = glm::cos(glm::radians(lookYaw));
		return glm::vec3(cp*sy, sp, cp*cy);
	}`

##### 2.1.3Maximum and minimum limits on yaw and pitch

`player.lookPitch = glm::clamp(player.lookPitch, -89.9f, 89.9f);//把player.lookPitch限定在 -89.9f, 89.9f之间`

## 3．Analysis of several structures

### 3.1 Player Structure

It mainly defines the relevant data of the player. Since this is the first-person perspective, the implementation of player and camera is written together.

`struct Player
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
} player;`

#### 3.1.1 Analysis of observation matrix in Player structure

Use the glm library to create a getViewMatrix () to return a 4th order matrix of the current perspective;

`glm::lookAt(pos, lookDir() + pos, glm::vec3(0, 1, 0));`

Parameter analysis:

pos：it is the position of the player (also camera) in the world coordinate system;

lookDir() + pos：The position of the object pointed by the camera lens in world coordinates = viewport orientation + position of player in world coordinates

glm::vec3(0, 1, 0))：Camera up direction in world coordinates

`glm::mat4 getViewMatrix()
	{
		pos.y = sightHeihgt;
		return glm::lookAt(pos, lookDir() + pos, glm::vec3(0, 1, 0));//eye center up lookAt为glm库自带的函数
	}`

#### 3.1.2 Analysis of Observation Direction in Player Structure

 It has been explained in "2.1.2.1 Conversion of angle".

### 3.2Explanation of related object parameters in the scene

#### 3.2.1Scene type

Using enumeration, 0 1 2 3 4 respectively represent: wall, box, ceiling, floor, box target points; convenient scene rendering later;

`enum CubeType
{
	Wall=0,	       
	Crate=1,		
	Ceiling=2,	
	Floor=3,		
	DestSpot,	
};`

#### 3.2.2Object status in the scene

glm::vec3 pos: The position of the object in the world coordinate system;

CubeType type：Cube type

bool inPosition：Whether it has been moved to the position (only meaningful for type == Crate) Only the box can be moved

unsigned char canMoveDir;//There are 4 directions that can be moved 0 1 2 3 ----> + x -x + z -z；Later used for collision detection and for direction identification

`struct MapCube
{
	glm::vec3 pos;	
	CubeType type;		
	bool inPosition;	
    unsigned char canMoveDir;		
};`

#### 3.2.3Box sliding related data

MapCube *cube：cube

glm::vec3 slideDir：Sliding direction

glm::vec3 destPos：Sliding target position

`struct CubeSlide
{
	MapCube *cube;	
	glm::vec3 slideDir;	
	glm::vec3 destPos;	
};`

## 4, load texture analysis

References an open source library for loading textures stb_image.h, which encapsulates functions and implements texture binding itself

`GLuint loadTexture(const char * filePath)
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
}`

## 5．Update of box moving status

First traverse the entire mapCubes, and start to perform state update judgment when it is retrieved as Crate. Determine whether the distance between the box and the surrounding objects is length = dis = 1.0001, if it is less than it means that the box is adjacent to the surrounding current objects; then judge the XZ direction The distance between the upper box and the surrounding objects is marked by the canMoveDir movement direction;

				`	~1  1 << 0  1   Right
					~2  1 << 1  2   Left
					~3  1 << 2  4   Down
					~4  1 << 3  8   Up `

`void physicsUpdate()
{
	for (int i=0;i<mapCubes.size();++i)
	{
		if (mapCubes[i].type!=Crate) continue;

		mapCubes[i].canMoveDir = 0xff;
	
		for (int j=0;j<mapCubes.size();++j)
		{
			glm::vec3 dis = mapCubes[i].pos - mapCubes[j].pos;
			if (length(dis)<=1.0001)
			{
				if (dis.x>0.5) mapCubes[i].canMoveDir &= ~1;       
				if (dis.x<-0.5) mapCubes[i].canMoveDir &= ~2;
				if (dis.z > 0.5) mapCubes[i].canMoveDir &= ~4;
				if (dis.z < -0.5) mapCubes[i].canMoveDir &= ~8;
			}
		}
	}
	for(int i=0;i<mapCubes.size();++i)
	{
		MapCube &cube=mapCubes[i];
		collisionPlayerBox(player, cube);
	}
}`

## 6．Player and cube collision detection

在void collisionPlayerBox(Player & player, MapCube & cube)函数中：

##### 1.collisionDistance is a constant representing the minimum collision distance:

`const float Player::collisionDistance = 0.6f;`

##### 2.First get the distance dis between the player and the cube, and reset the sliding direction to 0:

`	glm::vec3 dis = player.pos - cube.pos;
	glm::vec3 slideDir = glm::vec3(0);`

![](C:\Users\Andrew\Desktop\typora pic\4.png)

​                          Note: red circle is cube, blue circle is player, green circle is collisionDistanc

##### 3.Judge the distance between the player and the cube. Only when the distance between the two is less than the minimum collision distance can the collision be performed (here, the judgment is made in the two directions of X and Z, and one is indispensable):

`if (glm::abs(dis.x) < Player::collisionDistance && glm::abs(dis.z) < Player::collisionDistance)`

##### 4.If both directions are within the range of the minimum collision distance, then determine the relative position of the player and the cube. If the distance in the X direction is greater than the distance in the Z direction (player in the 1 4 5 8 quadrant), further accurate determination of the relative position is required. The positive and negative of the XZ axis and the radius r = glm :: abs (dis.z) / glm :: abs (dis.x) are used to make a comprehensive judgment; then the specific quadrant is obtained; the corresponding sliding direction is updated. Yes (only in the positive XZ direction of the box)

		`if (glm::abs(dis.x) > glm::abs(dis.z))
		{
			float r = glm::abs(dis.z) / glm::abs(dis.x);
			if (dis.x > 0)                                               
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
		}`
##### The complete code for this part:

`void collisionPlayerBox(Player & player, MapCube & cube)
{
	glm::vec3 dis = player.pos - cube.pos;
	glm::vec3 slideDir = glm::vec3(0);
	bool boxSlide = false;
	if (glm::abs(dis.x) < Player::collisionDistance && glm::abs(dis.z) < Player::collisionDistance)//collisionDistance  0.6  两个方向均小于最小碰撞距离
	{
		if (glm::abs(dis.x) > glm::abs(dis.z))
		{
			float r = glm::abs(dis.z) / glm::abs(dis.x);
			if (dis.x > 0)                                               
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
}`

## 7.Shader

Wrap shader-related programs in the `GLuint createShaderProgram ();` function.

I put the box's data in an array of boxDate of a float:

`float boxData[] = {
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
	};`

Created a vertex shader:

`	const char *vertSrc = R"glsl(
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
)glsl";`

Fragment shader:

`const char *fragSrc = R"glsl(
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

)glsl";`

General operation process (program part as an example):

##### 1.Create shader process:

1.1Call glCreateShader () to create a Shader object:

`GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);`

1.2Call glShaderSource () to create and load the Shader script source code:

`glShaderSource(vertShader, 1, &vertSrc, 0);`

1.3Call glCompileShader () to compile the Shader script:

`glCompileShader(vertShader);`

##### 2.The created Shader needs to be mounted into the Program. The process of creating a Program is as follows:

2.1Call glCreateProgram () to create a Program object:

`GLuint program = glCreateProgram();`

2.2Call glAttachShader () to mount the created shader:

`glAttachShader(program, vertShader);`

2.3Call glLinkProgram () to perform the link operation:

`glLinkProgram(program);`

2.4Switch the rendering pipeline to use the current shader for rendering

Complete code：

`GLuint createShaderProgram()
{
	const char *vertSrc = R"glsl(
\#version 330
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

\#version 330
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

​	float diff=max(0.0,dot(L,N));
​	float spec=pow(max(0.0,dot(H,N)),64.0);
​	
​	vec3 baseColor=texture(tex,fs_in.texCoord).rgb*tintColor;
​	color=vec4(0,0,0,1);
​	color.rgb=(0.3+diff*0.7)*baseColor*lightIntensity*attanuation;
​	//color.rgb=(0.1+diff)*baseColor*lightIntensity;

}

)glsl";

​	GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
​	GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
​	GLuint program = glCreateProgram();
​	
​	glShaderSource(vertShader, 1, &vertSrc, 0);
​	glCompileShader(vertShader);
​	glShaderSource(fragShader, 1, &fragSrc, 0);
​	glCompileShader(fragShader);
​	
​	glAttachShader(program, vertShader);
​	glAttachShader(program, fragShader);
​	
​	glLinkProgram(program);
​	
​	glDetachShader(program, vertShader);
​	glDetachShader(program, fragShader);
​	
​	glDeleteShader(vertShader);
​	glDeleteShader(fragShader);
​	
​	int logLen = 0;
​	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLen);
​	if (logLen)
​	{
​		char *info = new char[logLen + 1];
​		info[logLen] = 0;
​		glGetProgramInfoLog(program, logLen, 0, info);
​		printf("Program Error:\n%s", info);
​		delete[] info;
​	}
​	
​	return program;

}`



### 8.UML

#### 8.1Class Diagram

![](C:\Users\Andrew\Desktop\typora pic\cl.png)

#### 8.2Function call graph

![](C:\Users\Andrew\Desktop\typora pic\6.png)

In order of execution, from top to bottom

## Question three：What makes your program special and how does it compare to similar things? (Where did you get the idea from? What did you start with? How did you make yours unique?

This project started on October 12, 2019. When I browsed the production video of OpenGL on the Internet, I found that someone was trying to make a "Minecraft" game through OpenGL. His production process is very admirable and the game's reduction It's also very high, and I thought of using OpenGL to make a traditional game that was different from the past. At the beginning, I thought of "Super Mario Bros." I used to play as a child. I wanted to make a 3D "Super Mario Bros.", but was later told that making it was too difficult for me, so I turned my attention to the first game I've played since birth: "Push Box" I plan to make a push box. Unlike the traditional push box, I plan to make a push box with a first-person perspective.

