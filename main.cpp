#include <SDL2/SDL.h>
#include <SDL_events.h>
#include <SDL_render.h>
#include <cstdlib>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/geometric.hpp>
#include <string>
#include <glm/glm.hpp>
#include <vector>
#include "print.h"

#include "color.h"
#include "intersect.h"
#include "object.h"
#include "cube.h"
#include "light.h"
#include "camera.h"
#include "skybox.h"

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const float ASPECT_RATIO = static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT);
const int MAX_RECURSION = 3;
const float BIAS = 0.0001f;

SDL_Renderer* renderer;
std::vector<Object*> objects;
Light light(glm::vec3(-100.0f, -100.0f, -100.0f), 10.0f, Color(200, 0, 0));
Camera camera(glm::vec3(0.0f, 10.0f, 20.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 10.0f, 0.0f), 10.0f);
Skybox skybox("../assets/circuito.png");

void point(glm::vec2 position, Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawPoint(renderer, position.x, position.y);
}

float castShadow(const glm::vec3& shadowOrigin, const glm::vec3& lightDir, Object* hitObject) {
    for (auto& obj : objects) {
        if (obj != hitObject) {
            Intersect shadowIntersect = obj->rayIntersect(shadowOrigin, lightDir);
            if (shadowIntersect.isIntersecting && shadowIntersect.dist > 0) {
                float shadowRatio = shadowIntersect.dist / glm::length(light.position - shadowOrigin);
                shadowRatio = glm::min(1.0f, shadowRatio);

                // Check if the shadow point is in the same direction as the light
                float dotProduct = glm::dot(shadowIntersect.normal, -lightDir);
                if (dotProduct > 0.0f) {
                    return 1.0f - shadowRatio;
                }
            }
        }
    }
    return 1.0f;
}

Color castRay(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, const short recursion = 0) {
    float zBuffer = 99999;
    Object* hitObject = nullptr;
    Intersect intersect;

    for (const auto& object : objects) {
        Intersect i = object->rayIntersect(rayOrigin, rayDirection);
        if (i.isIntersecting && i.dist < zBuffer) {
            zBuffer = i.dist;
            hitObject = object;
            intersect = i;
        }
    }

    if (!intersect.isIntersecting || recursion == MAX_RECURSION) {
        return skybox.getColor(rayDirection);
    }


    glm::vec3 lightDir = glm::normalize(light.position - intersect.point);
    glm::vec3 viewDir = glm::normalize(rayOrigin - intersect.point);
    glm::vec3 reflectDir = glm::reflect(-lightDir, intersect.normal);

    float shadowIntensity = castShadow(intersect.point, lightDir, hitObject);

    float diffuseLightIntensity = std::max(0.0f, glm::dot(intersect.normal, lightDir));
    float specReflection = glm::dot(viewDir, reflectDir);

    Material mat = hitObject->material;

    float specLightIntensity = std::pow(std::max(0.0f, glm::dot(viewDir, reflectDir)), mat.specularCoefficient);


    Color reflectedColor(0.0f, 0.0f, 0.0f);
    if (mat.reflectivity > 0) {
        glm::vec3 origin = intersect.point + intersect.normal * BIAS;
        reflectedColor = castRay(origin, reflectDir, recursion + 1);
    }

    Color refractedColor(0.0f, 0.0f, 0.0f);
    if (mat.transparency > 0) {
        glm::vec3 origin = intersect.point - intersect.normal * BIAS;
        glm::vec3 refractDir = glm::refract(rayDirection, intersect.normal, mat.refractionIndex);
        refractedColor = castRay(origin, refractDir, recursion + 1);
    }


    Color diffuseLight = mat.diffuse * light.intensity * diffuseLightIntensity * mat.albedo * shadowIntensity;
    Color specularLight = light.color * light.intensity * specLightIntensity * mat.specularAlbedo * shadowIntensity;
    Color color = (diffuseLight + specularLight) * (1.0f - mat.reflectivity - mat.transparency) + reflectedColor * mat.reflectivity + refractedColor * mat.transparency;
    return color;
}

void setUp() {

    Material redMat = {
            Color(200, 0, 0),
            0.9f,
            0.9f,
            2.0f,
            0.4f,
            -1.0f,
    };


    Material traje = {
            Color(100, 100, 80),
            0.9f,
            0.5f,
            2.0f,
            0.0f,
            0.0f,
    };

    Material metalMaterial = {
            Color(192, 192, 192),   // Color difuso gris para simular metal
            0.8f,                   // Coeficiente albedo
            0.8f,                   // Coeficiente de albedo especular
            30.0f,                  // Coeficiente especular
            0.5f,                   // Reflectividad (puedes ajustar este valor)
            -1.0f,                   // Transparencia
    };

    Material bandera = {
            Color(255, 255, 255),
            0.7f,
            0.7f,
            100.0f,
            0.3f,
            -1.0f,
    };

    Material banderaNegra = {
            Color(0, 0, 0),
            0.9f,
            0.0f,
            2.0f,
            0.0f,
            -1.0f,
    };


    Material llantas = {
            Color(0, 0, 0),
            0.9f,
            0.0f,
            2.0f,
            0.0f,
            0.0f,
    };

    Material estructura = {
            Color(0, 0, 0),
            0.8f,
            0.0f,
            3.0f,
            0.0f,
            0.0f,
    };

    Material mirror = {
            Color(255, 255, 255),
            0.0f,
            10.0f,
            1425.0f,
            0.9f,
            0.0f
    };

    Material glass = {
            Color(201, 41, 41),
            0.0f,
            10.0f,
            1425.0f,
            0.2f,
            1.0f,
    };

    // Primer cubo Llanta
    objects.push_back(new Cube(glm::vec3(-2.5f, -1.0f, -2.5f), glm::vec3(-0.5f, 1.0f, -0.5f), llantas));

    // Segundo cubo al lado del primero
    objects.push_back(new Cube(glm::vec3(-0.5f, -1.0f, -2.5f), glm::vec3(2.5f, 1.0f, -0.5f), redMat));

    // Tercer cubo al lado de los dos anteriores LLanta
    objects.push_back(new Cube(glm::vec3(2.5f, -1.0f, -2.5f), glm::vec3(4.5f, 1.0f, -0.5f), llantas));

    // Cuarto cubo encima del segundo
    //objects.push_back(new Cube(glm::vec3(-0.5f, 1.0f, -2.5f), glm::vec3(2.0f, 3.0f, -0.5f), glass));

    // Cuarto cubo enfrente del primero Aleron
    objects.push_back(new Cube(glm::vec3(-3.0f, -1.0f, 2.0f), glm::vec3(-0.5f, -0.5f, 0.5f), redMat));
    objects.push_back(new Cube(glm::vec3(-3.0f, -1.0f, 2.0f), glm::vec3(-2.7f, 0.0f, 0.5f), redMat));

    // Quinto cubo enfrente del segundo entre aleron
    objects.push_back(new Cube(glm::vec3(-0.5f, -1.0f, 2.0f), glm::vec3(2.5f, 1.0f, -0.5f), redMat));

    // Sexto cubo enfrente del tercero Aleron
    objects.push_back(new Cube(glm::vec3(2.5f, -1.0f, 2.0f), glm::vec3(5.0f, -0.5f, 0.5f), redMat));
    objects.push_back(new Cube(glm::vec3(4.7f, -1.0f, 2.0f), glm::vec3(5.0f, 0.0f, 0.5f), redMat));

    // Séptimo cubo detrás del segundo
    objects.push_back(new Cube(glm::vec3(-0.5f, -1.0f, -4.5f), glm::vec3(2.5f, 1.0f, -2.5f), redMat));
    //Octavo detras del septimo
    objects.push_back(new Cube(glm::vec3(-0.5f, -1.0f, -6.5f), glm::vec3(2.5f, 1.0f, -3.0f), redMat));
    //Lados del octavo
    objects.push_back(new Cube(glm::vec3(-1.5f, -1.0f, -6.5f), glm::vec3(-0.5f, 1.0f, -3.0f), redMat));
    objects.push_back(new Cube(glm::vec3(3.5f, -1.0f, -6.5f), glm::vec3(2.5f, 1.0f, -3.0f), redMat));

    //Lados del asiento
    objects.push_back(new Cube(glm::vec3(-1.5f, -1.0f, -8.5f), glm::vec3(-0.5f, 1.0f, -6.5f), redMat));
    objects.push_back(new Cube(glm::vec3(3.5f, -1.0f, -8.5f), glm::vec3(2.5f, 1.0f, -6.5f), redMat));
    //Piloto
    objects.push_back(new Cube(glm::vec3(0.5f, -1.0f, -8.0f), glm::vec3(1.5f, 1.0f, -7.0f), traje));

    //Casco
    objects.push_back(new Cube(glm::vec3(0.5f, 1.0f, -8.0f), glm::vec3(0.7f, 2.0f, -7.0f), estructura));
    objects.push_back(new Cube(glm::vec3(1.3f, 1.0f, -8.0f), glm::vec3(1.5f, 2.0f, -7.0f), estructura));
    objects.push_back(new Cube(glm::vec3(0.5f, 1.8f, -8.0f), glm::vec3(1.5f, 2.0f, -7.0f), estructura));
    objects.push_back(new Cube(glm::vec3(0.5f, 1.0f, -8.0f), glm::vec3(1.5f, 1.3f, -7.0f), estructura));
    objects.push_back(new Cube(glm::vec3(0.7f, 1.3f, -8.0f), glm::vec3(1.3f, 1.8f, -8.0f), estructura));
    objects.push_back(new Cube(glm::vec3(0.7f, 1.3f, -8.0f), glm::vec3(1.3f, 1.8f, -7.0f), mirror));

    //Noveno detras de octavo
    objects.push_back(new Cube(glm::vec3(-0.5f, -1.0f, -10.5f), glm::vec3(2.5f, 1.0f, -8.5f), redMat));
    //Lados del noveno
    objects.push_back(new Cube(glm::vec3(-1.5f, -1.0f, -10.0f), glm::vec3(-0.5f, 1.0f, -8.5f), redMat));
    objects.push_back(new Cube(glm::vec3(3.5f, -1.0f, -10.0f), glm::vec3(2.5f, 1.0f, -8.5f), redMat));
    //Decimo detras de noveno entre llantas
    objects.push_back(new Cube(glm::vec3(-0.5f, -1.0f, -12.5f), glm::vec3(2.5f, 1.0f, -10.5f), redMat));
    //Lantas Traseras
    objects.push_back(new Cube(glm::vec3(-2.5f, -1.0f, -12.5f), glm::vec3(-0.5f, 1.0f, -10.5f), estructura));
    objects.push_back(new Cube(glm::vec3(2.5f, -1.0f, -12.5f), glm::vec3(4.5f, 1.0f, -10.5f), estructura));
    //Onceavo detras de decimo
    objects.push_back(new Cube(glm::vec3(-0.5f, -1.0f, -13.5f), glm::vec3(2.5f, 1.0f, -11.5f), redMat));
    //Soporte DRS encima de onceavo
    objects.push_back(new Cube(glm::vec3(-0.0f, -1.0f, -13.5f), glm::vec3(0.5f, 1.5f, -13.0f), estructura));
    objects.push_back(new Cube(glm::vec3(1.5f, -1.0f, -13.5f), glm::vec3(2.0f, 1.5f, -13.0f), estructura));
    //DRS
    objects.push_back(new Cube(glm::vec3(-1.5f, 1.5f, -14.5f), glm::vec3(3.5f, 1.8f, -13.0f), redMat));

    //Tubo izquierdo
    objects.push_back(new Cube(glm::vec3(-8.5f, -1.0f, -4.5f), glm::vec3(-7.5f, 10.0f, -4.0f), metalMaterial));
    //Tubo derecho
    objects.push_back(new Cube(glm::vec3(8.5f, -1.0f, -4.5f), glm::vec3(9.5f, 10.0f, -4.0f), metalMaterial));

    //Meta
    objects.push_back(new Cube(glm::vec3(-8.5f, 7.0f, -4.5f), glm::vec3(9.5f, 10.0f, -4.0f), bandera));

    //Bandera negro adelante
    objects.push_back(new Cube(glm::vec3(-7.3f, 9.0f, -5.0f), glm::vec3(-6.3f, 10.0f, -3.5f), banderaNegra));
    objects.push_back(new Cube(glm::vec3(-5.3f, 9.0f, -5.0f), glm::vec3(-4.3f, 10.0f, -3.5f), banderaNegra));
    objects.push_back(new Cube(glm::vec3(-3.3f, 9.0f, -5.0f), glm::vec3(-2.3f, 10.0f, -3.5f), banderaNegra));
    objects.push_back(new Cube(glm::vec3(-1.3f, 9.0f, -5.0f), glm::vec3(0.3f, 10.0f, -3.5f), banderaNegra));
    objects.push_back(new Cube(glm::vec3(1.3f, 9.0f, -5.0f), glm::vec3(2.3f, 10.0f, -3.5f), banderaNegra));
    objects.push_back(new Cube(glm::vec3(3.3f, 9.0f, -5.0f), glm::vec3(4.3f, 10.0f, -3.5f), banderaNegra));
    objects.push_back(new Cube(glm::vec3(5.3f, 9.0f, -5.0f), glm::vec3(6.3f, 10.0f, -3.5f), banderaNegra));
    objects.push_back(new Cube(glm::vec3(7.3f, 9.0f, -5.0f), glm::vec3(8.3f, 10.0f, -3.5f), banderaNegra));

    objects.push_back(new Cube(glm::vec3(-7.3f, 7.0f, -5.0f), glm::vec3(-6.3f, 8.0f, -3.5f), banderaNegra));
    objects.push_back(new Cube(glm::vec3(-5.3f, 7.0f, -5.0f), glm::vec3(-4.3f, 8.0f, -3.5f), banderaNegra));
    objects.push_back(new Cube(glm::vec3(-3.3f, 7.0f, -5.0f), glm::vec3(-2.3f, 8.0f, -3.5f), banderaNegra));
    objects.push_back(new Cube(glm::vec3(-1.3f, 7.0f, -5.0f), glm::vec3(0.3f, 8.0f, -3.5f), banderaNegra));
    objects.push_back(new Cube(glm::vec3(1.3f, 7.0f, -5.0f), glm::vec3(2.3f, 8.0f, -3.5f), banderaNegra));
    objects.push_back(new Cube(glm::vec3(3.3f, 7.0f, -5.0f), glm::vec3(4.3f, 8.0f, -3.5f), banderaNegra));
    objects.push_back(new Cube(glm::vec3(5.3f, 7.0f, -5.0f), glm::vec3(6.3f, 8.0f, -3.5f), banderaNegra));
    objects.push_back(new Cube(glm::vec3(7.3f, 7.0f, -5.0f), glm::vec3(8.3f, 8.0f, -3.5f), banderaNegra));

    objects.push_back(new Cube(glm::vec3(-6.3f, 8.0f, -5.0f), glm::vec3(-5.3f, 9.0f, -3.5f), banderaNegra));
    objects.push_back(new Cube(glm::vec3(-4.3f, 8.0f, -5.0f), glm::vec3(-3.3f, 9.0f, -3.5f), banderaNegra));
    objects.push_back(new Cube(glm::vec3(-2.3f, 8.0f, -5.0f), glm::vec3(-1.3f, 9.0f, -3.5f), banderaNegra));
    objects.push_back(new Cube(glm::vec3(0.3f, 8.0f, -5.0f), glm::vec3(1.3f, 9.0f, -3.5f), banderaNegra));
    objects.push_back(new Cube(glm::vec3(2.3f, 8.0f, -5.0f), glm::vec3(3.3f, 9.0f, -3.5f), banderaNegra));
    objects.push_back(new Cube(glm::vec3(4.3f, 8.0f, -5.0f), glm::vec3(5.3f, 9.0f, -3.5f), banderaNegra));
    objects.push_back(new Cube(glm::vec3(6.3f, 8.0f, -5.0f), glm::vec3(7.3f, 9.0f, -3.5f), banderaNegra));


}


void render() {
    float fov = 3.1415/3;
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            /*
            float random_value = static_cast<float>(std::rand())/static_cast<float>(RAND_MAX);
            if (random_value < 0.0) {
                continue;
            }
            */
            float screenX = (2.0f * (x + 0.5f)) / SCREEN_WIDTH - 1.0f;
            float screenY = -(2.0f * (y + 0.5f)) / SCREEN_HEIGHT + 1.0f;
            screenX *= ASPECT_RATIO;
            screenX *= tan(fov/2.0f);
            screenY *= tan(fov/2.0f);

            glm::vec3 cameraDir = glm::normalize(camera.target - camera.position);

            glm::vec3 cameraX = glm::normalize(glm::cross(cameraDir, camera.up));
            glm::vec3 cameraY = glm::normalize(glm::cross(cameraX, cameraDir));
            glm::vec3 rayDirection = glm::normalize(
                cameraDir + cameraX * screenX + cameraY * screenY
            );
           
            Color pixelColor = castRay(camera.position, rayDirection);
            /* Color pixelColor = castRay(glm::vec3(0,0,20), glm::normalize(glm::vec3(screenX, screenY, -1.0f))); */

            point(glm::vec2(x, y), pixelColor);
        }
    }
}

int main(int argc, char* argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        return 1;
    }

    // Create a window
    SDL_Window* window = SDL_CreateWindow("Hello World - FPS: 0", 
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                                          SCREEN_WIDTH, SCREEN_HEIGHT, 
                                          SDL_WINDOW_SHOWN);

    if (!window) {
        SDL_Log("Unable to create window: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Create a renderer
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (!renderer) {
        SDL_Log("Unable to create renderer: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    bool running = true;
    SDL_Event event;

    int frameCount = 0;
    Uint32 startTime = SDL_GetTicks();
    Uint32 currentTime = startTime;
    
    setUp();

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }

            if (event.type == SDL_KEYDOWN) {
                switch(event.key.keysym.sym) {
                    case SDLK_UP:
                        camera.move(-1.0f);
                        break;
                    case SDLK_DOWN:
                        camera.move(1.0f);
                        break;
                    case SDLK_LEFT:
                        print("left");
                        camera.rotate(-1.0f, 0.0f);
                        break;
                    case SDLK_RIGHT:
                        print("right");
                        camera.rotate(1.0f, 0.0f);
                        break;
                    case SDLK_w:
                        camera.moveVertical(1.0f);
                        break;
                    case SDLK_s:
                        camera.moveVertical(-1.0f);
                        break;
                 }
            }
        }

        // Clear the screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        render();

        // Present the renderer
        SDL_RenderPresent(renderer);

        frameCount++;

        // Calculate and display FPS
        if (SDL_GetTicks() - currentTime >= 1000) {
            currentTime = SDL_GetTicks();
            std::string title = "Raytracer F1 - FPS: " + std::to_string(frameCount);
            SDL_SetWindowTitle(window, title.c_str());
            frameCount = 0;
        }
    }

    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}