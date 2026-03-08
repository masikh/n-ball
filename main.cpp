#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <cmath>
#include <vector>
#include <algorithm>

static constexpr double PI = 3.14159265358979323846;
static constexpr int    CIRCLE_SEGMENTS = 128;
static constexpr int    N_MAX = 25;
static constexpr float  SPACING = 0.55f;

static int   winW = 1100, winH = 750;
static float camAngleX = 25.0f;
static float camAngleY = 0.0f;
static float camDist   = 14.0f;
static bool  autoRotate = true;
static float sceneRotX = 0.0f;
static float sceneRotZ = 0.0f;
static double lastMouseX, lastMouseY;
static bool  dragging = false;

// f(n) = pi^(n/2) / Gamma(n/2 + 1) — volume of the unit n-ball
static double f(double n) {
    return std::pow(PI, n / 2.0) / std::tgamma(n / 2.0 + 1.0);
}

// ── HSV to RGB helper ──
static void hsvToRgb(float h, float s, float v, float &r, float &g, float &b) {
    int   i = (int)std::floor(h * 6.0f);
    float f = h * 6.0f - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - f * s);
    float t = v * (1.0f - (1.0f - f) * s);
    switch (i % 6) {
        case 0: r=v; g=t; b=p; break;
        case 1: r=q; g=v; b=p; break;
        case 2: r=p; g=v; b=t; break;
        case 3: r=p; g=q; b=v; break;
        case 4: r=t; g=p; b=v; break;
        case 5: r=v; g=p; b=q; break;
    }
}

// ── Draw a circle ring in the XZ plane at a given Y, with a given radius ──
static void drawCircleRing(float y, float radius, float r, float g, float b, float alpha, float lineW) {
    glLineWidth(lineW);
    glColor4f(r, g, b, alpha);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < CIRCLE_SEGMENTS; i++) {
        float theta = 2.0f * PI * i / CIRCLE_SEGMENTS;
        glVertex3f(radius * std::cos(theta), y, radius * std::sin(theta));
    }
    glEnd();
}

// ── Draw a filled translucent disc in the XZ plane ──
static void drawFilledDisc(float y, float radius, float r, float g, float b, float alpha) {
    glColor4f(r, g, b, alpha);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0.0f, y, 0.0f);
    for (int i = 0; i <= CIRCLE_SEGMENTS; i++) {
        float theta = 2.0f * PI * i / CIRCLE_SEGMENTS;
        glVertex3f(radius * std::cos(theta), y, radius * std::sin(theta));
    }
    glEnd();
}

// ── Draw a thin vertical connector line from one circle level to the next ──
static void drawConnector(float y0, float y1, float r0, float r1, float angle,
                          float cr, float cg, float cb) {
    glColor4f(cr, cg, cb, 0.25f);
    glBegin(GL_LINES);
    glVertex3f(r0 * std::cos(angle), y0, r0 * std::sin(angle));
    glVertex3f(r1 * std::cos(angle), y1, r1 * std::sin(angle));
    glEnd();
}

// ── Draw the vertical axis spine ──
static void drawAxis(float yMin, float yMax) {
    glLineWidth(1.5f);
    glColor4f(0.5f, 0.5f, 0.6f, 0.6f);
    glBegin(GL_LINES);
    glVertex3f(0, yMin, 0);
    glVertex3f(0, yMax, 0);
    glEnd();
}

// ── Draw a ground grid ──
static void drawGroundGrid(float y, float extent, int divisions) {
    glLineWidth(1.0f);
    glColor4f(0.2f, 0.2f, 0.25f, 0.3f);
    glBegin(GL_LINES);
    float step = 2.0f * extent / divisions;
    for (int i = 0; i <= divisions; i++) {
        float p = -extent + i * step;
        glVertex3f(p, y, -extent);
        glVertex3f(p, y,  extent);
        glVertex3f(-extent, y, p);
        glVertex3f( extent, y, p);
    }
    glEnd();
}

// ── Draw an envelope curve connecting the outer edges of the circles ──
static void drawEnvelope(int nMax, float spacing) {
    glLineWidth(2.0f);
    glColor4f(1.0f, 1.0f, 1.0f, 0.15f);
    // Front envelope
    glBegin(GL_LINE_STRIP);
    for (int n = 0; n <= nMax; n++) {
        float radius = (float)f(n);
        float y = n * spacing;
        glVertex3f(radius, y, 0.0f);
    }
    glEnd();
    // Back envelope
    glBegin(GL_LINE_STRIP);
    for (int n = 0; n <= nMax; n++) {
        float radius = (float)f(n);
        float y = n * spacing;
        glVertex3f(-radius, y, 0.0f);
    }
    glEnd();
    // Side envelopes
    glBegin(GL_LINE_STRIP);
    for (int n = 0; n <= nMax; n++) {
        float radius = (float)f(n);
        float y = n * spacing;
        glVertex3f(0.0f, y, radius);
    }
    glEnd();
    glBegin(GL_LINE_STRIP);
    for (int n = 0; n <= nMax; n++) {
        float radius = (float)f(n);
        float y = n * spacing;
        glVertex3f(0.0f, y, -radius);
    }
    glEnd();
}

// ── Draw interpolated circles between integer n values for smoothness ──
static void drawInterpolatedCircles(int nMax, float spacing, float time) {
    constexpr int INTERP_STEPS = 4;
    for (int n = 0; n < nMax; n++) {
        for (int s = 1; s < INTERP_STEPS; s++) {
            float frac = (float)s / INTERP_STEPS;
            float nCont = n + frac;
            float radius = (float)f(nCont);
            float y = nCont * spacing;
            float t = nCont / (float)nMax;
            float r, g, b;
            hsvToRgb(0.55f + t * 0.45f, 0.5f, 0.7f, r, g, b);
            drawCircleRing(y, radius, r, g, b, 0.12f, 1.0f);
        }
    }
}

static void drawScene(float time) {
    // Ground grid below everything
    drawGroundGrid(-0.5f, 7.0f, 14);

    // Vertical axis
    drawAxis(-0.5f, (N_MAX + 1) * SPACING);

    // Envelope curves
    drawEnvelope(N_MAX, SPACING);

    // Interpolated circles (subtle fill between integers)
    drawInterpolatedCircles(N_MAX, SPACING, time);

    // Main integer circles
    for (int n = 0; n <= N_MAX; n++) {
        float radius = (float)f(n);
        float y = n * SPACING;
        float t = (float)n / N_MAX;

        float r, g, b;
        hsvToRgb(0.55f + t * 0.45f, 0.8f, 1.0f, r, g, b);

        // Filled translucent disc
        drawFilledDisc(y, radius, r, g, b, 0.08f + 0.04f * std::sin(time + n * 0.5f));

        // Bright outer ring
        float pulse = 1.5f + 0.5f * std::sin(time * 2.0f + n * 0.3f);
        drawCircleRing(y, radius, r, g, b, 0.85f, pulse + 1.0f);

        // Inner glow ring
        drawCircleRing(y, radius * 0.98f, r * 0.7f, g * 0.7f, b * 0.7f, 0.3f, 1.0f);

        // Connectors to next circle
        if (n < N_MAX) {
            float nextR = (float)f(n + 1);
            float nextY = (n + 1) * SPACING;
            for (int k = 0; k < 8; k++) {
                float angle = 2.0f * PI * k / 8.0f;
                drawConnector(y, nextY, radius, nextR, angle, r, g, b);
            }
        }
    }

    // Highlight the peak circle (n=5, where volume is maximum among integers)
    {
        int peakN = 5;
        float peakR = (float)f(peakN);
        float peakY = peakN * SPACING;
        float glow = 0.6f + 0.4f * std::sin(time * 3.0f);
        drawCircleRing(peakY, peakR + 0.05f, 1.0f, 0.9f, 0.2f, glow, 3.0f);
        drawCircleRing(peakY, peakR + 0.10f, 1.0f, 0.8f, 0.1f, glow * 0.4f, 1.5f);
    }
}

// ── GLFW callbacks ──
static void framebufferSizeCallback(GLFWwindow *, int w, int h) {
    winW = w; winH = h;
    glViewport(0, 0, w, h);
}

static void scrollCallback(GLFWwindow *, double, double yoff) {
    camDist -= (float)yoff * 0.8f;
    camDist = std::clamp(camDist, 4.0f, 30.0f);
}

static void mouseButtonCallback(GLFWwindow *win, int button, int action, int) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        dragging = (action == GLFW_PRESS);
        if (dragging) {
            glfwGetCursorPos(win, &lastMouseX, &lastMouseY);
            autoRotate = false;
        }
    }
}

static void cursorPosCallback(GLFWwindow *, double x, double y) {
    if (dragging) {
        float dx = (float)(x - lastMouseX);
        float dy = (float)(y - lastMouseY);
        camAngleY += dx * 0.3f;
        camAngleX += dy * 0.3f;
        camAngleX = std::clamp(camAngleX, -89.0f, 89.0f);
        lastMouseX = x;
        lastMouseY = y;
    }
}

static void keyCallback(GLFWwindow *win, int key, int, int action, int) {
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(win, GLFW_TRUE);
        if (key == GLFW_KEY_SPACE) autoRotate = !autoRotate;
        if (key == GLFW_KEY_R) { camAngleX = 25.0f; camAngleY = 0.0f; camDist = 14.0f; sceneRotX = 0.0f; sceneRotZ = 0.0f; autoRotate = true; }
    }
}

int main() {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow *window = glfwCreateWindow(winW, winH,
        "pi^(n/2) / (n/2)!  —  3D Unit n-Ball Volumes", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetKeyCallback(window, keyCallback);

    glfwGetFramebufferSize(window, &winW, &winH);
    glViewport(0, 0, winW, winH);

    glEnable(GL_MULTISAMPLE);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POINT_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    while (!glfwWindowShouldClose(window)) {
        float time = (float)glfwGetTime();

        if (autoRotate) {
            camAngleY += 0.25f;
            sceneRotX += 0.07f;
            sceneRotZ += 0.04f;
        }

        glClearColor(0.04f, 0.04f, 0.07f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Perspective projection
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        float aspect = (float)winW / std::max((float)winH, 1.0f);
        gluPerspective(45.0, aspect, 0.1, 100.0);

        // Camera
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        float centerY = N_MAX * SPACING * 0.4f;
        float radX = camAngleX * PI / 180.0f;
        float radY = camAngleY * PI / 180.0f;
        float eyeX = camDist * std::cos(radX) * std::sin(radY);
        float eyeY = centerY + camDist * std::sin(radX);
        float eyeZ = camDist * std::cos(radX) * std::cos(radY);
        gluLookAt(eyeX, eyeY, eyeZ, 0, centerY, 0, 0, 1, 0);

        // Two slower secondary rotations around the scene center
        glTranslatef(0, centerY, 0);
        glRotatef(sceneRotX, 1.0f, 0.0f, 0.0f);
        glRotatef(sceneRotZ, 0.0f, 0.0f, 1.0f);
        glTranslatef(0, -centerY, 0);

        // Draw transparent objects back-to-front by disabling depth-write for discs
        glDepthMask(GL_FALSE);
        drawScene(time);
        glDepthMask(GL_TRUE);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}