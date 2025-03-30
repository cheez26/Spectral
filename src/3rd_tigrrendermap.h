#define WINDOW_WIDTH _320
#define WINDOW_HEIGHT _240

#define tigrMin(a,b) ((a) < (b) ? (a) : (b))
#define tigrMax(a,b) ((a) > (b) ? (a) : (b))
#define tigrClamp(v,a,b) ((v) < (a) ? (a) : (v) > (b) ? (b) : (v))

// Bilinear interpolation helper function
TPixel bilinearSample(Tigr* src, float x, float y) {
    int x0 = (int)x;
    int y0 = (int)y;
    int x1 = x0 + 1;
    int y1 = y0 + 1;
    
    x0 = tigrClamp(x0, 0, src->w - 1);
    x1 = tigrClamp(x1, 0, src->w - 1);
    y0 = tigrClamp(y0, 0, src->h - 1);
    y1 = tigrClamp(y1, 0, src->h - 1);
    
    TPixel p00 = src->pix[y0 * src->w + x0];
    TPixel p10 = src->pix[y0 * src->w + x1];
    TPixel p01 = src->pix[y1 * src->w + x0];
    TPixel p11 = src->pix[y1 * src->w + x1];
    
    float fx = x - x0;
    float fy = y - y0;
    float fx1 = 1.0f - fx;
    float fy1 = 1.0f - fy;
    
    TPixel result;
    result.r = (unsigned char)(fx1 * fy1 * p00.r + fx * fy1 * p10.r + fx1 * fy * p01.r + fx * fy * p11.r);
    result.g = (unsigned char)(fx1 * fy1 * p00.g + fx * fy1 * p10.g + fx1 * fy * p01.g + fx * fy * p11.g);
    result.b = (unsigned char)(fx1 * fy1 * p00.b + fx * fy1 * p10.b + fx1 * fy * p01.b + fx * fy * p11.b);
    result.a = 255;
    
    return result;
}

void getScaleFactors(Tigr *dest, Tigr *map, float zoom, float minZoom, float *scaleX, float *scaleY) {
    // Base scale factors for full canvas fit
    float fullScaleX = (float)map->w / dest->w;  // e.g., 2048/800 = 2.56
    float fullScaleY = (float)map->h / dest->h;  // e.g., 2048/600 = 3.413
    // Square scale for map's original aspect ratio
    float mapRatio = 1.0f; // (float)map->w / map->h;
    float mapScale = mapRatio / zoom;
    // Smooth interpolation factor: 0 at minZoom, 1 at minZoom * 2
    float transitionEnd = minZoom * 2.0f;
    float t = (zoom - minZoom) / (transitionEnd - minZoom);
    t = tigrClamp(t, 0.0f, 1.0f);
    // Interpolate scales
    *scaleX = fullScaleX * (1-t) + mapScale * t;
    *scaleY = fullScaleY * (1-t) + mapScale * t;
    // At minimum zoom, force exact fit
    if (zoom <= minZoom + 0.001f) {
        *scaleX = fullScaleX;
        *scaleY = fullScaleY;
    }
}

// Scaling function with smooth transition
void scaleBlit(Tigr* dest, Tigr* src, float centerX, float centerY, float zoom, float minZoom, float maxZoom) {
    // Base scale factors for full canvas fit
    float scaleX, scaleY;
    getScaleFactors(dest, src, zoom, minZoom, &scaleX, &scaleY);

    for (int y = 0; y < dest->h; y++) {
        for (int x = 0; x < dest->w; x++) {
            float srcX = centerX + (x - dest->w/2) * scaleX;
            float srcY = centerY + (y - dest->h/2) * scaleY;
            
            if (srcX >= 0 && srcX < src->w - 1 && srcY >= 0 && srcY < src->h - 1) {
                dest->pix[y * dest->w + x] = bilinearSample(src, srcX, srcY);
            } else if (srcX >= 0 && srcX < src->w && srcY >= 0 && srcY < src->h) {
                int sx = (int)srcX;
                int sy = (int)srcY;
                dest->pix[y * dest->w + x] = src->pix[sy * src->w + sx];
            }
        }
    }
}

void tigrRenderMap(Tigr *screen, Tigr *map, int mx, int my, int buttons, float wheel) {
    static int   dragging, lastX, lastY;
    static float zoom, minZoom, maxZoom;
    static float centerX, centerY;

    static int once = 1; if(!screen && !map) { once = 1; return; }
    if( once ) {
        once = 0;

        dragging = 0;
        lastX = 0;
        lastY = 0;

        // Calculate minimum zoom where texture fits canvas
        float mapAspect = (float)map->w / map->h;
        float windowAspect = (float)WINDOW_WIDTH / WINDOW_HEIGHT;
        minZoom = (mapAspect > windowAspect) ?  (float)map->w / WINDOW_WIDTH : (float)map->h / WINDOW_HEIGHT;
        maxZoom = minZoom * 8.f; // x8 max

        zoom = 1.0f;
        centerX = map->w/2; //1024.0f;
        centerY = map->h/2; //1024.0f;
    }

    // Panning: Disable at minZoom, otherwise clamp to map bounds
    if (buttons & 1 && zoom > minZoom + 0.001f) {
        if (!dragging) {
            dragging = 1;
            lastX = mx;
            lastY = my;
        } else {
            float dx = (mx - lastX); // / zoom;
            float dy = (my - lastY); // / zoom;
            centerX -= dx;
            centerY -= dy;
            lastX = mx;
            lastY = my;
        }
    } else {
        dragging = 0;
    }

    if (wheel != 0) {
        // Calculate current scale factors before zoom change
        float oldScaleX, oldScaleY;
        getScaleFactors(screen, map, zoom, minZoom, &oldScaleX, &oldScaleY);

        // Mouse position in map coordinates before zoom
        float mouseMapX = centerX + (mx - WINDOW_WIDTH/2) * oldScaleX;
        float mouseMapY = centerY + (my - WINDOW_HEIGHT/2) * oldScaleY;

        // Adjust zoom
        float zoomFactor = (wheel > 0) ? 1.01f : (1.0f / 1.01f);
        zoom = tigrClamp(zoom * zoomFactor, minZoom, maxZoom);

        // Recalculate scale factors after zoom change
        float newScaleX, newScaleY;
        getScaleFactors(screen, map, zoom, minZoom, &newScaleX, &newScaleY);

        // Adjust center to keep mouse position stable
        centerX = mouseMapX - (mx - WINDOW_WIDTH/2) * newScaleX;
        centerY = mouseMapY - (my - WINDOW_HEIGHT/2) * newScaleY;
    }

    // Calculate view size in map coordinates
    float scaleX, scaleY;
    getScaleFactors(screen, map, zoom, minZoom, &scaleX, &scaleY);

    float viewWidth = WINDOW_WIDTH * scaleX;
    float viewHeight = WINDOW_HEIGHT * scaleY;

    // Clamp center to keep view within map bounds
    float minX = viewWidth / 2;
    float maxX = map->w - viewWidth / 2;
    float minY = viewHeight / 2;
    float maxY = map->h - viewHeight / 2;
    centerX = tigrClamp(centerX, minX, maxX);
    centerY = tigrClamp(centerY, minY, maxY);

    scaleBlit(screen, map, centerX, centerY, zoom, minZoom, maxZoom);

    enum { _10 = 3 };
    TPixel red = tigrRGB(255, 0, 0);
    tigrLine(screen, mx - _10, my, mx + _10 + 1, my, red);
    tigrLine(screen, mx, my - _10, mx, my + _10 + 1, red);
}

void tigrRenderInitMap(void) {
    tigrRenderMap(NULL, NULL, 0,0,0,0);
}
