#include <cmath>
#include "SDL_image.h"
#include "Image.h"
#include <iostream>

Image::Image(SDL_Surface* surface, float ratio)
{

	/*This function converts an SDL surface directly to an openGL texture, rather than using the SDL_Renderer.
	 * 
	 * "SDL2's accelerated rendering is nothing more than a software wrapper around OpenGL or Direct3D.  
	 *  By using OpenGL you bypass the wrapper and get to use the underlying API directly, which can 
	 *  give you more control but at the expense of greater complexity."
	 * 
	 */



	GLenum texture_format;
	GLint channels, internalformat;

	SDL_Surface* originalSurface = surface;
	
	int h = pow(2, ceil(log(originalSurface->h) / log(2))); // Round up to the nearest power of two
	int newh = h;
	int neww = ceil(float(originalSurface->w)*(float(h) / float(originalSurface->h)));
	int w = pow(2, ceil(log(neww) / log(2))); // Round up to the nearest power of two


	while (h > SCREEN_HEIGHT || w > SCREEN_WIDTH)
	{
		h = h / 2;
		w = w / 2;
		newh = newh / 2;
		neww = neww / 2;
	}


	//std::cerr << "  Image w: " << w << " h: " << h << std::endl;
	
	SDL_Rect stretchRect;
	stretchRect.x = floor((w - neww) / 2.0f);
	stretchRect.y = floor((h - newh) / 2.0f);
	stretchRect.w = neww;
	stretchRect.h = newh;

	SDL_Surface* newSurface;
	newSurface = SDL_CreateRGBSurface(0, w, h, surface->format->BitsPerPixel, surface->format->Rmask, surface->format->Gmask, surface->format->Bmask, surface->format->Amask);
	SDL_FillRect(newSurface, NULL, SDL_MapRGB(newSurface->format, 255, 255, 255));
	SDL_BlitScaled(originalSurface, NULL, newSurface, &stretchRect); // Blit onto a purely RGB Surface
	surface = newSurface;

	channels = surface->format->BytesPerPixel;
	if (channels == 4) // contains an alpha channel
	{
		if (surface->format->Rmask == 0x000000ff)
		{
			texture_format = GL_RGBA; 
			//internalformat = GL_RGBA8;
			//newSurface = SDL_CreateRGBSurface(0, w, h, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
		}
		else
		{
			texture_format = GL_BGRA;
			//internalformat = GL_RGBA8;
			//newSurface = SDL_CreateRGBSurface(0, w, h, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
		}
	}
	else // no alpha channel
	{
		if (surface->format->Rmask == 0x000000ff)
		{
			texture_format = GL_RGB;
			//internalformat = GL_RGB8;
			//newSurface = SDL_CreateRGBSurface(0, w, h, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);

		}
		else
		{
			texture_format = GL_BGR;
			//internalformat = GL_RGB8;
			//newSurface = SDL_CreateRGBSurface(0, w, h, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
		}
	}


	GLenum error;

	error = glGetError();

	while ( error != GL_NO_ERROR)
	{
		error = glGetError();
	}


	// generate the OpenGL texture and store the width and height
	glGenTextures(1, &texture);
	if ((error = glGetError()) != GL_NO_ERROR)
	{
		std::cerr << "GL Gen Texture error. " << error << " texnum: " << texture << std::endl;
	}

	glBindTexture(GL_TEXTURE_2D, texture);
	if ((error = glGetError()) != GL_NO_ERROR)
	{
		std::cerr << "GL Binding error." << error << std::endl;
	}


	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	if ((error = glGetError()) != GL_NO_ERROR)
	{
		std::cerr << "GL Tex2 error." << error << std::endl;
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if ((error = glGetError()) != GL_NO_ERROR)
	{
		std::cerr << "GL Tex3 error." << error << std::endl;
	}


	//std::cerr << "surf: " << surface->w << " " << surface->h << " " << texture_format << " " << surface->pixels << std::endl;

	glTexImage2D(GL_TEXTURE_2D, 0, channels, surface->w, surface->h, 0, texture_format, GL_UNSIGNED_BYTE, surface->pixels);  //GL_UNSIGNED_INT

	if ((error = glGetError()) != GL_NO_ERROR)
	{
		std::cerr << "GL Tex1 error." << error << std::endl;
	}


	width = (GLfloat)surface->w * ratio;
	height = (GLfloat)surface->h * ratio;

	drawOn = 1;
}

Image* Image::LoadFromFile(char* filePath)
{
	SDL_Surface* surface = IMG_Load(filePath);

	GLenum error;

	if ((error = glGetError()) != GL_NO_ERROR)
	{
		std::cerr << "GL error. " << error << std::endl;
	}

	if (surface == NULL) // failed to load file
	{
		std::cerr << "Image failed to load." << std::endl;
		return NULL;
	}
	else
	{
		Image* image = new Image(surface);
		SDL_FreeSurface(surface);
		return image;
	}
}

GLuint Image::GetTexture() const
{
	return texture;
}

GLfloat Image::GetWidth() const
{
	return width;
}

GLfloat Image::GetHeight() const
{
	return height;
}

void Image::Draw(GLfloat xPos, GLfloat yPos, GLfloat theta)
{
	Draw(xPos, yPos, width, height, theta);
}

void Image::Draw(GLfloat xPos, GLfloat yPos, GLfloat w, GLfloat h, GLfloat theta)
{	

	if (drawOn)
	{

		glBindTexture(GL_TEXTURE_2D, texture);
		glEnable(GL_TEXTURE_2D);

		// Draw a quad with the texture on it
		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f);
		//glColor3f(1.0f, 1.0f, 1.0f);
		glVertex3f(xPos - (w / 2) * cos(theta) - (h / 2) * sin(theta),
			yPos - (w / 2) * sin(theta) + (h / 2) * cos(theta), 0.0f);
		glTexCoord2f(1.0f, 0.0f);
		glVertex3f(xPos + (w / 2) * cos(theta) - (h / 2) * sin(theta),
			yPos + (w / 2) * sin(theta) + (h / 2) * cos(theta), 0.0f);
		glTexCoord2f(1.0f, 1.0f);
		glVertex3f(xPos + (w / 2) * cos(theta) + (h / 2) * sin(theta),
			yPos + (w / 2) * sin(theta) - (h / 2) * cos(theta), 0.0f);
		glTexCoord2f(0.0f, 1.0f);
		glVertex3f(xPos - (w / 2) * cos(theta) + (h / 2) * sin(theta),
			yPos - (w / 2) * sin(theta) - (h / 2) * cos(theta), 0.0f);
		glEnd();

		//glColor3f(1.0f, 1.0f, 1.0f);

		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);

	}
}

void Image::DrawAlign(GLfloat xPos, GLfloat yPos, GLint cflag)
{
	DrawAlign(xPos, yPos, width, height, cflag);
}


void Image::DrawAlign(GLfloat xPos, GLfloat yPos, GLfloat w, GLfloat h, GLint cflag)
{
	if (drawOn)
	{

		glBindTexture(GL_TEXTURE_2D, texture);
		glEnable(GL_TEXTURE_2D);

		if (cflag == 1)  //right-align
		{
			// Draw a quad with the texture on it
			glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(xPos - w, yPos + (h / 2), 0.0f);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3f(xPos,     yPos + (h / 2), 0.0f);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(xPos,     yPos - (h / 2), 0.0f);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3f(xPos - w, yPos - (h / 2), 0.0f);
			glEnd();
		}
		else if (cflag == 2) //top-align
		{
			// Draw a quad with the texture on it
			glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(xPos - (w / 2), yPos, 0.0f);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3f(xPos + (w / 2), yPos, 0.0f);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(xPos + (w / 2), yPos - h,     0.0f);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3f(xPos - (w / 2), yPos - h,     0.0f);
			glEnd();

		}
		else if (cflag == 3) //left-align 
		{

			// Draw a quad with the texture on it
			glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(xPos,	 yPos + (h / 2), 0.0f);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3f(xPos + w, yPos + (h / 2), 0.0f);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(xPos + w, yPos - (h / 2), 0.0f);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3f(xPos,     yPos - (h / 2), 0.0f);
			glEnd();

		}
		else if (cflag == 4)  //bottom-align
		{
			// Draw a quad with the texture on it
			glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(xPos - (w / 2), yPos + h, 0.0f);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3f(xPos + (w / 2), yPos + h, 0.0f);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(xPos + (w / 2), yPos,     0.0f);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3f(xPos - (w / 2), yPos,     0.0f);
			glEnd();
		}

		else
		{
			// Draw a quad with the texture on it
			glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(xPos - (w / 2), yPos + (h / 2), 0.0f);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3f(xPos + (w / 2), yPos + (h / 2), 0.0f);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(xPos + (w / 2), yPos - (h / 2), 0.0f);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3f(xPos - (w / 2), yPos - (h / 2), 0.0f);
			glEnd();
		}

		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);

	}

}

void Image::On()
{
	drawOn = 1;
}

void Image::Off()
{
	drawOn = 0;
}

int Image::DrawState()
{
	return(drawOn);
}



Image* Image::ImageText(Image* txt, const char* txtstr, const std::string& fonttype, int fontsize, SDL_Color fontcolor)
{
	/* To create text, call a render function from SDL_ttf and use it to create
	 * an Image object. See http://www.libsdl.org/projects/SDL_ttf/docs/SDL_ttf.html#SEC42
	 * for a list of render functions.
	 *
	 * Note, this method has not been updated for new SDL2 functionality, but it still works.
	 */


	delete txt;

	TTF_Font *font;

	const std::string strdir = "Resources/";
	const std::string fontstr = strdir + fonttype;
	std::cerr << fontstr << std::endl;

	font = TTF_OpenFont(fontstr.c_str(), fontsize);
	//std::cerr << font << std::endl;

	SDL_Surface *text_surface;

	
	if (!(text_surface = TTF_RenderText_Blended(font, txtstr, fontcolor)))
	{
		std::cerr << "Text creation failed." << std::endl;
	}
	//else
	//	std::cerr << "Text created." << std::endl;

	GLenum error;

	if ((error = glGetError()) != GL_NO_ERROR)
	{
		std::cerr << "GL textrender error. " << error << std::endl;
	}

	txt = new Image(text_surface);


	if ((error = glGetError()) != GL_NO_ERROR)
	{
		std::cerr << "GL imagetext error. " << error << std::endl;
	}

	std::cerr << fontstr << std::endl;

	TTF_CloseFont(font);

	return(txt);

}