// @fixme: excessive va()

#include "res/scr/question_mark"

zxdb ZXDB2;

rgba* thumbnail(const byte *VRAM_, int len, unsigned downfactor, int ZXFlashFlag) {
    int w = 256 / downfactor, h = 192 / downfactor;

    rgba *texture = malloc( w * h * 4 ), *cpy = texture;
    if( len != 6912 && len != (6912+64) && len != (6144+768*4) && len != (6144+768*8) )
        return texture; // @fixme: ula+/.ifl/.mlt 12k(T),9k(Z)oomblox

    if( DEV && len == (6912+64) )
        alert(va("vram: %d bytes found", len));

    #define SCANLINE_(y) \
        ((((((y)%64) & 0x38) >> 3 | (((y)%64) & 0x07) << 3) + ((y)/64) * 64) << 5)

    rgba *ZXPalette = ZXPalettes[0]; // [5] B/W palette for a good noir effect!

    for( int y = 0; y < 192; y += downfactor ) {
        // paper
        const byte *pixels=VRAM_+SCANLINE_(y);
        const byte *attribs=VRAM_+6144+((y&0xF8)<<2);
        rgba *bak = texture;

        for(int x = 0; x < 32; ++x ) {
            byte attr = *attribs;
            byte pixel = *pixels, fg, bg;

            // @fixme: make section branchless

            pixel ^= (attr & 0x80) && ZXFlashFlag ? 0xff : 0x00;
            fg = (attr & 0x07) | ((attr & 0x40) >> 3);
            bg = (attr & 0x78) >> 3;

            if( downfactor == 1 ) {
            texture[0]=ZXPalette[pixel & 0x80 ? fg : bg];
            texture[1]=ZXPalette[pixel & 0x40 ? fg : bg];
            texture[2]=ZXPalette[pixel & 0x20 ? fg : bg];
            texture[3]=ZXPalette[pixel & 0x10 ? fg : bg];
            texture[4]=ZXPalette[pixel & 0x08 ? fg : bg];
            texture[5]=ZXPalette[pixel & 0x04 ? fg : bg];
            texture[6]=ZXPalette[pixel & 0x02 ? fg : bg];
            texture[7]=ZXPalette[pixel & 0x01 ? fg : bg];
            texture += 8;
            }
            else if( downfactor == 2 ) {
            texture[0]=ZXPalette[pixel & 0x80 ? fg : bg];
            texture[1]=ZXPalette[pixel & 0x20 ? fg : bg];
            texture[2]=ZXPalette[pixel & 0x08 ? fg : bg];
            texture[3]=ZXPalette[pixel & 0x02 ? fg : bg];
            texture += 4;
            }
            else if( downfactor == 4 ) {
            texture[0]=ZXPalette[pixel & 0x80 ? fg : bg];
            texture[1]=ZXPalette[pixel & 0x08 ? fg : bg];
            texture += 2;
            }
            else if( downfactor == 8 ) {
            texture[0]=ZXPalette[pixel & 0x80 ? fg : bg];
            texture += 1;
            }

            pixels++;
            attribs++;
        }

        texture = bak + w;
    }

    return cpy;
}



static
const char *tab;

static
int zxdb_compare_by_name(const void *arg1, const void *arg2) { // @fixme: roman
    char **a = (char**)*(VAL**)arg1; char *entry = *a;
    char **b = (char**)*(VAL**)arg2; char *other = *b;

    char *year1  = strchr(entry,  '|')+1;
    char *title1 = strchr(year1,  '|')+1;
    char *alias1 = strchr(title1, '|')+1;

    char *year2  = strchr(other,  '|')+1;
    char *title2 = strchr(year2,  '|')+1;
    char *alias2 = strchr(title2, '|')+1;

    if( tab ) {
        if( *tab == '#' ) {
            if( *alias1 != '|' && (isdigit(*alias1) || ispunct(*alias1)) ) title1 = alias1;
            if( *alias2 != '|' && (isdigit(*alias2) || ispunct(*alias2)) ) title2 = alias2;
        } else {
            if( *title1 != *tab && *alias1 == *tab ) title1 = alias1;
            if( *title2 != *tab && *alias2 == *tab ) title2 = alias2;
        }
    }

    return strcmpi(title1, title2);
}

char *zxdb_screen(const char *id, int *len) {
    if( id && id[0] && strcmp(id, "0") && strcmp(id, "#") ) {
        ZXDB2 = zxdb_search( id );

        if( ZXDB2.ids[0] ) {
            static char *data = 0;
            if( data ) free(data), data = 0;
            if(!data ) data = zxdb_download(ZXDB2,zxdb_url(ZXDB2, "screen"), len);
            if(!data ) data = zxdb_download(ZXDB2,zxdb_url(ZXDB2, "running"), len);
            return data;
        }
    }
    return NULL;
}

bool zxdb_load(const char *id, int ZX_MODEL) {
    if( id && id[0] && strcmp(id, "0") && strcmp(id, "#") ) {

        const char *hint = "play";

#if 0
        // convert #game-id#release-seq format into {game,seq} pair
        for( int gid, seq; sscanf(id, "#%d#%d", &gid, &seq ) == 2; ) {
            static char clean[16], hints[16];
            snprintf(clean, 15, "#%d", gid); id = clean;
            snprintf(hints, 15, "#%d", seq); hint = hints+1;
            break;
        }
#else
        for( int gid, seq; sscanf(id, "#%d#%d", &gid, &seq ) == 2; ) {
            hint = strrchr(id,'#')+1;
            *strrchr(id,'#') = '\0';
            break;
        }
#endif


        ZXDB2 = zxdb_search( id );

        if( ZXDB2.ids[0] ) {
            int len;

            static char *data = 0;
            if( data ) free(data), data = 0;
            if(!data ) data = zxdb_download(ZXDB2,zxdb_url(ZXDB2, hint), &len);
            if( data ) {

                if( ZX_MODEL ) {
                    boot(ZX = ZX_MODEL, ~0u);
                }
                else {
                    ZX_PENTAGON = 0;
                    char *model = strchr(ZXDB2.ids[5], ',')+1;
                    /**/ if( strstr(model, "Pentagon") ) boot(ZX = 128, ~0u), ZX_PENTAGON = 1, rom_restore();
                    else if( strstr(model, "+3") )       boot(ZX = 300, ~0u);
                    else if( strstr(model, "+2A") )      boot(ZX = 210, ~0u);
                    else if( strstr(model, "+2B") )      boot(ZX = 210, ~0u);
                    else if( strstr(model, "+2") )       boot(ZX = 200, ~0u);
                    else if( strstr(model, "USR0") )     boot(ZX = 128, ~0u); // @fixme
                    else if( strstr(model, "128") )      boot(ZX = 128, ~0u);
                    else if( strstr(model, "48") )       boot(ZX = 48 /*+ 80 * (atoi(ZXDB2.ids[1]) >= 1987)*/, ~0u);
                    else                                 boot(ZX = 16, ~0u);
                }

        #if 0
                loadbin(data, len, true);
        #else
                // this temp file is a hack for now. @fixme: move the zip/rar/fdi loaders into loadbin()
                for( FILE *fp = fopen("spectral.$$2", "wb"); fp; fwrite(data, len, 1, fp), fclose(fp), fp = 0) {
                }
                loadfile("spectral.$$2", 1);
                unlink("spectral.$$2");
        #endif
            }

            ZXDB = ZXDB2;

            return true; // @fixme: verify that previous step went right
        }
    }
    return false;
}

bool zxdb_reload(int ZX_MODEL) {
    return ZXDB.ids[0] ? zxdb_load(va("#%s", ZXDB.ids[0]), ZX_MODEL) : false;
}







#pragma pack(push, 1)
typedef struct cache_t {
    uint16_t likes : 4;
    uint16_t flags : 4;
    uint16_t reserved : 8;
} cache_t;
#pragma pack(pop)

typedef int static_assert_cache_t[sizeof(cache_t) == 2];

enum {
    CACHE_TRANSFER = 2, // if download is in progress

    CACHE_MP3 = 1,
    CACHE_TXT = 2,
    CACHE_POK = 4,
    CACHE_SCR = 8,
    CACHE_SNA = 16,
    CACHE_JPG = 32,
    CACHE_MAP = 64,
};

uint16_t *cache;

void cache_load() {
    if(!cache) {
        cache = calloc(2, 65536); // zxdb_count());
        for( FILE *fp = fopen(".Spectral/Spectral.fav", "rb"); fp; fclose(fp), fp = 0) {
            fread(cache, 2 * 65536, 1, fp);
        }
    }
}
void cache_save() {
    for( FILE *fp = fopen(".Spectral/Spectral.fav", "wb"); fp; fclose(fp), fp = 0) {
        fwrite(cache, 2 * 65536, 1, fp);
    }
}
uint16_t cache_get(unsigned zxdb) {
    cache_load();
    return cache[zxdb];
}
uint16_t cache_set(unsigned zxdb, uint16_t v) {
    cache_load();
    int changed = cache[zxdb] ^ v;
    cache[zxdb] = v;
    if( changed ) cache_save();
    return v;
}

// screens + thumbnails
// 64K ZXDB entries max, x2 flash versions (on/off) each, x4 versions each (1:1,2:1,4:1,8:1 shrinks)
const byte* screens[65536][2][4];
unsigned short screens_len[65536];


thread_ptr_t worker, worker2;
thread_queue_t queue, queue2;
struct queue_t {
    zxdb z;
    char *url;
};
void* queue_values[144]; // 12x12 thumbnails max
void* queue_values2[144]; // 12x12 thumbnails max
struct queue_t *queue_t_new(zxdb z,const char *url) {
    struct queue_t *q = malloc(sizeof(struct queue_t));
    q->z = zxdb_dup(z);
    q->url = url ? strdup(url) : NULL;
    return q;
}
double worker_progress() {
    unsigned capacity = sizeof(queue_values) / sizeof(queue_values[0]);
    unsigned count = worker ? thread_queue_count(&queue) : 0;
    return 1 - (count / (double)capacity);
}
int worker_fn( void* userdata ) {
    for(;;) {
        void* item = thread_queue_consume((thread_queue_t*)userdata, THREAD_QUEUE_WAIT_INFINITE);
        struct queue_t *q = (struct queue_t*)item;
        printf("queue recv %s\n", q->url);

        int id = atoi(q->z.ids[0]), len = 0;
        if( screens_len[id] == 1 ) {
            char *bin = zxdb_download(q->z, q->url, &len);
            if( !bin || !len ) {
                // black screen
                // bin = calloc(1,len = 6912);
                bin = (char*)question_mark;
                len = question_mark_length;
            }

            {
                screens[id][0][0] =
                screens[id][0][1] =
                screens[id][0][2] =
                screens[id][0][3] =
                screens[id][1][0] =
                screens[id][1][1] =
                screens[id][1][2] =
                screens[id][1][3] = (const byte*)bin;

                int ix,iy,in;
                rgba *bitmap = (rgba*)stbi_load_from_memory(bin, len, &ix, &iy, &in, 4);
                if( bitmap ) {
                    screens[id][0][1] = screens[id][1][1] = (const byte*)ui_resize(bitmap, ix, iy, 256/2, 192/2, 1, 0);
                    screens[id][0][2] = screens[id][1][2] = (const byte*)ui_resize(bitmap, ix, iy, 256/4, 192/4, 0, 0);
                    screens[id][0][3] = screens[id][1][3] = (const byte*)ui_resize(bitmap, ix, iy, 256/8, 192/8, 0, 0);
                    stbi_image_free(bitmap);
                } else {
                    bitmap = thumbnail(bin, len, 1, 0); ix = 256, iy = 192;
                    screens[id][0][1] = screens[id][1][1] = (const byte*)ui_resize(bitmap, ix, iy, 256/2, 192/2, 1, 0);
                    screens[id][0][2] = screens[id][1][2] = (const byte*)ui_resize(bitmap, ix, iy, 256/4, 192/4, 0, 0);
                    screens[id][0][3] = screens[id][1][3] = (const byte*)ui_resize(bitmap, ix, iy, 256/8, 192/8, 0, 0);
                    free(bitmap);

                    for( int i = 0; i < 768; ++i) {
                        int has_flash = ((byte*)bin)[i] & 0x80;
                        if( has_flash ) {
                            bitmap = thumbnail(bin, len, 1, 1); ix = 256, iy = 192;
                            screens[id][1][1] = (const byte*)ui_resize(bitmap, ix, iy, 256/2, 192/2, 1, 0);
                            screens[id][1][2] = (const byte*)ui_resize(bitmap, ix, iy, 256/4, 192/4, 0, 0);
                            screens[id][1][3] = (const byte*)ui_resize(bitmap, ix, iy, 256/8, 192/8, 0, 0);
                            free(bitmap);
                            break;
                        }
                    }
                }

                screens_len[id] = len;
            }
        }

        zxdb_free(q->z);
        free(q->url);
        free(q);
    }
    return 0;
}
int worker_push(const zxdb z, const char *url, int ms) {
    unsigned hash = url ? fnv1a(url, strlen(url)) : (unsigned)(uintptr_t)url;
    unsigned bucket = hash & 1;

    if( hash & 1 ) {
        // init
        int capacity = sizeof(queue_values) / sizeof(queue_values[0]);
        if(!worker)  thread_queue_init(&queue, capacity, queue_values, 0);
        if(!worker)  thread_detach( worker = thread_init(worker_fn, &queue, "worker_fn", THREAD_STACK_SIZE_DEFAULT) );

        if( thread_queue_count(&queue) < capacity )
            if( thread_queue_produce(&queue, queue_t_new(z,url), ms ) ) // THREAD_QUEUE_WAIT_INFINITE );
                return printf("queue send1 %s\n", url), 1;
    } else {
        // init2
        int capacity2 = sizeof(queue_values2) / sizeof(queue_values2[0]);
        if(!worker2) thread_queue_init(&queue2, capacity2, queue_values2, 0);
        if(!worker2) thread_detach( worker2 = thread_init(worker_fn, &queue2, "worker_fn2", THREAD_STACK_SIZE_DEFAULT) );

        if( thread_queue_count(&queue2) < capacity2 )
            if( thread_queue_produce(&queue2, queue_t_new(z,url), ms ) ) // THREAD_QUEUE_WAIT_INFINITE );
                return printf("queue send2 %s\n", url), 1;
    }
    return 0;
}

const
char *zxdb_screen_async(const char *id, int *len, int factor) {
    if( id && id[0] && strcmp(id, "0") && strcmp(id, "#") ) {
        ZXDB2 = zxdb_search( id );

        if( ZXDB2.ids[0] ) {
            int zxdb_id = atoi(ZXDB2.ids[0]);
            if( screens_len[zxdb_id] == 0 ) {
                char *url = 0;
                if(!url ) url = zxdb_url(ZXDB2, "screen");
                if(!url ) url = zxdb_url(ZXDB2, "running");
                if( worker_push(ZXDB2, url, 1) )
                    screens_len[zxdb_id] = 1;
            }
            if( screens_len[zxdb_id] > 1 ) {
                return *len = screens_len[zxdb_id], screens[zxdb_id][ZXFlashFlag][factor & 3];
            }
        }
    }
    return NULL;
}









int active; // @todo: rename to browsing, browser_active, or library_active

extern Tigr *app, *ui;
extern char *last_load;

char **games;
int *dbgames;
int numgames;
int numok,numwarn,numerr; // stats
void rescan(const char *folder) {
    if(!folder) return;
    if(ZX_PLAYER) return; // zxplayer has no library

    printf("scanning `%s` folder...\n", folder);

    // convert to absolute
    char buffer[MAX_PATH]={0};
    realpath(folder, buffer);
    folder = buffer;

    char parent[MAX_PATH]={0};
    snprintf(parent, MAX_PATH, "%s/../", folder);

    // clean up
    while( numgames ) free(games[--numgames]);
    games = realloc(games, 0);

    // refresh stats
    {
        numok=0,numwarn=0,numerr=0;

        // add parent folder `..`
        ++numgames;
        games = realloc(games, numgames * sizeof(char*) );
        games[numgames-1] = strdup(parent);
        dbgames = realloc(dbgames, numgames * sizeof(char*) );
        dbgames[numgames-1] = 0;

        for( dir *d = dir_open(folder, NULL); d; dir_close(d), d = NULL ) {
            for( unsigned is_file = 0; is_file < 2; ++is_file )
            for( unsigned count = 0, end = dir_count(d); count < end; ++count ) {
                if( is_file ^ dir_file(d,count) ) continue;

                const char *fname = dir_name(d, count);
                if( is_file ? file_is_supported(fname,ALL_FILES) : 1 ) {
                    // append
                    ++numgames;
                    games = realloc(games, numgames * sizeof(char*) );
                    games[numgames-1] = strdup(fname);
                    //
                    dbgames = realloc(dbgames, numgames * sizeof(char*) );
                    dbgames[numgames-1] = is_file ? db_get(fname) : 0;
                }
                if( is_file && strendi(fname, ".db") ) {
                    for(FILE *fp2 = fopen(fname, "rb"); fp2; fclose(fp2), fp2=0) {
                        int ch;
                        fscanf(fp2, "%d", &ch); ch &= 0xFF;
                        numok += ch == 1;
                        numerr += ch == 2;
                        numwarn += ch == 3;
                    }
                }
            }
        }
    }

    printf("%d games\n", numgames);
}
void draw_compatibility_stats(window *layer) {
    // compatibility stats
    int total = numok+numwarn+numerr;
    if(total && active) {
    TPixel white = {255,255,255,255}, black = {0,0,0,255}, *bar = &ui->pix[0 + _239 * _320];
    int num1 = (numok * (float)_319) / total;
    int num2 = (numwarn * (float)_319) / total;
    int num3 = (numerr * (float)_319) / total; if((num1+num2+num3)<_319) num1 += _319 - (num1+num2+num3);
    for( int x = 0; x <= num1; ++x ) bar[x-320]=bar[x] = tigrRGB(64,255,64);
    for( int x = 0; x <= num2; ++x ) bar[x+num1-320]=bar[x+num1] = tigrRGB(255,192,64);
    for( int x = 0; x <= num3; ++x ) bar[x+num1+num2-320]=bar[x+num1+num2] = tigrRGB(255,64,64);
    static char compat[64];
    snprintf(compat, 64, "  OK:%04.1f%%     ENTER:128, +SHIFT:48, +CTRL:Try turbo", (total-numerr) * 100.f / (total+!total));
    window_printxy(layer, compat, 0,(_240-12.0)/11);
    }
}


char wildcard[256];
int  *queries; int numqueries;
VAL **remotes; int numremotes;
void search_query_v1(const char *query) {
    wildcard[0] = 0;

    numqueries = 0;
    queries = REALLOC((void*)queries, (query && query[0]) * sizeof(int) * 65536);
    if( !queries ) return;

    if( strchr(query, '*') )
    snprintf(wildcard, 256-1, "%s", query);
    else
    snprintf(wildcard, 256-1, "*%s*", query);

    // search local files for matches
    for( int i = 0; i < numgames; ++i ) {
        if( strmatchi(games[i],wildcard) ) queries[numqueries++] = i;
    }

    // search online files for matches
#if 1
        // search & sort
        free(remotes), remotes = 0;
        remotes = map_multifind(&zxdb2, wildcard, &numremotes);
        if( numremotes ) qsort(remotes, numremotes, sizeof(VAL*), zxdb_compare_by_name);

        // remove dupes (like aliases)
        for( int i = 1; i < numremotes; ++i ) {
            if( *(char**)remotes[i-1] == *(char**)remotes[i] ) {
                memmove(remotes + i - 1, remotes + i, ( numremotes - i ) * sizeof(remotes[0]));
                --numremotes;
            }
        }

        // exclude XXX games
        // exclude For(S)ale,(N)everReleased,Dupes(*),MIA(?) [include (A)vailable,(R)ecovered,(D)enied games]
        // exclude demos 72..78
        for( int i = 0; i < numremotes; ++i ) {
            char *zx_id = (char*)*remotes[i];
            char *years = strchr(zx_id, '|')+1; int zx_id_len = years-zx_id-1;
            char *title = strchr(years, '|')+1; int years_len = title-years-1;
            char *alias = strchr(title, '|')+1; int title_len = alias-title-1;
            char *brand = strchr(alias, '|')+1; int alias_len = brand-alias-1;
            char *avail = strchr(brand, '|')+1; int brand_len = avail-brand-1;
            char *score = strchr(avail, '|')+1; int avail_len = score-avail-1;
            char *genre = strchr(score, '|')+1; int score_len = genre-score-1;
            char *tags_ = strchr(genre, '|')+1; int genre_len = tags_-genre-1;

            if( avail[1] == 'X' || !strchr("ARD", avail[0]) || atoi(genre) >= 72 ) {
                memmove(remotes + i, remotes + i + 1, ( numremotes - i - 1 ) * sizeof(remotes[0]));
                --numremotes;
                --i;
            }
        }
#endif
}
char *search_results_v1() {
    if( !queries ) return NULL;

    extern int cmdkey;
    extern const char *cmdarg;

    ui_at(ui, 8*1, 11*2);

    for( int j = 0; j < numqueries; ++j ) {
        int i = queries[j];

        const char *sep = strrchr(games[i], *DIR_SEP_);
        int is_dir = sep[1] == '\0', is_file = !is_dir;
        if( is_dir ) continue;

        // @fixme: scan folder
        if( ui_click("Browse to folder", "\6" FOLDER_STR "\f\f") ) cmdkey = 'SCAN', cmdarg = va("%.*s",(int)(sep - games[i]), games[i]); // @todo: browse to containing folder
        if( ui_click(games[i], sep+1) ) return games[i];
        if( ui_click(NULL, "\n") );
    }

    for( int j = 0; j < numremotes; ++j ) {
        int i = j;

        char *zx_id = (char*)*remotes[i];
        char *years = strchr(zx_id, '|')+1; int zx_id_len = years-zx_id-1;
        char *title = strchr(years, '|')+1; int years_len = title-years-1;
        char *alias = strchr(title, '|')+1; int title_len = alias-title-1;
        char *brand = strchr(alias, '|')+1; int alias_len = brand-alias-1;
        char *avail = strchr(brand, '|')+1; int brand_len = avail-brand-1;
        char *score = strchr(avail, '|')+1; int avail_len = score-avail-1;
        char *genre = strchr(score, '|')+1; int score_len = genre-score-1;
        char *tags_ = strchr(genre, '|')+1; int genre_len = tags_-genre-1;

        // should we look into alias or main title
        char *main = va("%.*s", alias_len ? title_len : alias_len, alias_len ? title : alias);
        char *alt  = va("%.*s", alias_len ? alias_len : title_len, alias_len ? alias : title);
        int use_alt = !strmatchi(main, wildcard) && strmatchi(alt, wildcard);
        if(!use_alt) use_alt = i && atoi((char*)*remotes[i]) == atoi((char*)*remotes[i-1]);
        if( use_alt ) {
            char *swap = main;
            main = alt;
            alt = swap;
        }

        // replace year if title was never released
        if( years[0] == '9' ) years = "?", years_len = 1; // "9999"

        // replace brand if no brand is given. use 1st author if possible
        if( brand[0] == '|' ) {
            char *next = strchr(zx_id, '\n');
            if( next && next[1] == '@' ) { // x3 skips: '\n' + '@' + 'R'ole
                brand = next+1+1+1, brand_len = strcspn(brand, "@\r\n");
            }
        }

        // build full title and clean it up
        char full[64];
        snprintf(full, sizeof(full), " %s (%.*s)(%.*s)", main, years_len, years, brand_len, brand);
        for( int i = 1; full[i]; ++i )
            if( i == 1 || full[i-1] == '.' )
                full[i] = toupper(full[i]);

        char url[128];
        snprintf(url, 128-1, "Open link\nhttps://spectrumcomputing.co.uk/entry/%.*s", zx_id_len, zx_id);

        if( ui_click(url, "\x19\f\f") ) cmdkey = 'LINK', cmdarg = va("%s", url + countof("Open link\n"));
        if( ui_click(alt, full+1) ) active = 0, cmdkey = 'ZXDB', cmdarg = va("#%.*s", zx_id_len, zx_id);
        if( ui_click(NULL, "\n") ); ui_y--;
    }

    return NULL;
}


char *filter; // current text filter in use. can be NULL
int game_filter() { // returns true if any key was pressed
    enum { _16 = 32, _15 = _16 - 1 };
    static int chars[_16] = {0};
    static char utf8[5+_16*6+1] = "Hint:";
    filter = utf8 + 5;

        int chars_count = 0;
        while(chars[chars_count] && chars_count < _16) chars_count++;

    int anykey = 0, done = 0, clear = 0;
    // Grab any chars and add them to our buffer.
    for(;;) {
        int c = tigrReadChar(app);
        if (c == 0) break;
        if( window_pressed(app,TK_CONTROL)) break;
        if( c == '\n' || c == '\r' ) { done = 1; break; }
        if( window_pressed(app, TK_ESCAPE) ) { done = 1; clear = 1; break; } // memset(chars, 0, sizeof(int)*_16); chars_count = -1; break; }
        if( c == '\t' && chars_count > 0 ) { anykey = 1; break; } // ?
        if( c <  32 && c != '\b' ) continue;
        if( c == 32 && chars_count == 0 ) continue;
        if( c == 32 && chars_count >  0 && chars[chars_count-1] == 32 ) continue;
        
        anykey = 1;

        if( c != '\b' )
            chars[ (chars_count = min(chars_count+1,_15)) - 1 ] = c;
        else
        if( chars_count > 0 )
            chars[ --chars_count ] = 0;

        char *p = filter;
        for (int n=0;n<_16;n++)
            p = tigrEncodeUTF8(p, chars[n]);
        *p = 0;
    }

    // display

    int visible = num_options == 1 && (options[0].flags & 2) && options[0].text[0] == 'H';

    if( done ) {
        memset(chars, 0, sizeof(int)*_16);

        if( clear ) *filter = '\0';

        ui_dialog_new(NULL);
    }
    else {
        if( !visible && anykey ) {
            ui_dialog_new(NULL);
            ui_dialog_option(2, va("<%s▁                             \n",utf8),NULL, 0,NULL);
        }
        if( visible && window_longpress(app, TK_BACKSPACE) ) {
            memset(chars, chars_count = 0, sizeof(int)*_16); *filter = 0; anykey = 1;
        }
        if( visible && anykey ) {
            (void)REALLOC((void*)options[0].text, 0);
            options[0].text = STRDUP(va("%s▁\n",utf8));
        }
    }

    return anykey;
}



int selected, scroll;

int game_browser_keyboard(const int ENTRIES, const int numgames) { // returns clicked entry, or <0 if none
    if( scroll < 0 ) scroll = 0;
    if (!numgames) return -1;

#if 0
    if( window_pressed(app, TK_CONTROL) && window_trigger(app, TK_HOME) )
        return selected = 0, scroll = 0, -1;
    if( window_pressed(app, TK_CONTROL) && window_trigger(app, TK_END) )
        return selected = numgames % ENTRIES, scroll = numgames / ENTRIES, -1; //fixme
#endif

    static int tbl[256] = {0};
    int up = window_keyrepeat_(app, TK_DOWN, tbl) - window_keyrepeat_(app, TK_UP, tbl);
    int pg = window_keyrepeat_(app, TK_PAGEDN, tbl) - window_keyrepeat_(app, TK_PAGEUP, tbl);
    int home = window_pressed(app, TK_HOME) || (window_pressed(app, TK_UP) && tigrKeyHeld(app, TK_CONTROL));
    int end  = window_pressed(app, TK_END) || (window_pressed(app, TK_DOWN) && tigrKeyHeld(app, TK_CONTROL));

    float wheel = ifdef(osx, +, -) tigrMouseWheel(app);
    if( wheel ) {
        if( window_pressed(app, TK_CONTROL) ) {
            pg = wheel > 0 ? 1 : -1;
        }
        else {
            scroll += wheel;
        }
    }

    scroll = CLAMP(scroll, 0, numgames-1);
    selected = CLAMP(selected, 0, numgames-1);

    int top = scroll, bottom = scroll + ENTRIES - 1;
    if( pg == -1 ) if( selected != top    ) selected = top;    else selected = (scroll -= ENTRIES);
    if( pg == +1 ) if( selected != bottom ) selected = bottom; else selected = (scroll += ENTRIES) + ENTRIES - 1;
    if( up == -1 ) if( selected != top    ) --selected; else --selected, --scroll;
    if( up == +1 ) if( selected != bottom ) ++selected; else ++selected, ++scroll;

    if( home ) scroll = selected = 0;
    if( end  ) scroll = selected = numgames-1, scroll -= ENTRIES - 1;

    scroll = CLAMP(scroll, 0, numgames-1);
    selected = CLAMP(selected, 0, numgames-1);

    // update filter
    int has_finder = !!num_options;
    int any = game_filter();

    if( window_trigger(app, TK_RETURN) && !has_finder ) {
        return selected;
    }

    return -1;
}

char* game_browser_v1() {
    int press_backspace = tigrKeyDown(app, TK_BACKSPACE);
    int has_finder = !!num_options;

    enum { ENTRIES = (_240/11)-2 };
    int chosen = game_browser_keyboard(ENTRIES, numgames);
        // returns '..' if finder is not visible
        if( press_backspace && !has_finder ) return selected = 0, scroll = 0, games[0];
        // returns game selection
        if( chosen >= 0 ) {
            // if chosen is a dir, reset scroll&cursor for next frame
            if( strendi(games[chosen], "/") ) {
                scroll = 0, selected = 0;
            }
            // returns selection
            return games[chosen];
        }

    static char *buffer = 0; if(!buffer) { buffer = malloc(65536); /*rescan();*/ }

    int clicked = 0;

    int y = 0;
    for( int j = 0; j < ENTRIES; ++j ) {
        int i = scroll + j;
        if( i < 0 ) continue;
        if( i >= numgames ) continue;

        const char *sep = strrchr(games[i], *DIR_SEP_);
        int is_dir = sep[1] == '\0', is_file = !is_dir;
        if( is_dir ) while(0[--sep] != '/');

        int flagged = 0;
        int starred = 0;
        if( i == selected ) {
        if( window_pressed(app, TK_SHIFT) && window_trigger(app, TK_SPACE) ) flagged = 1;
        if(!window_pressed(app, TK_SHIFT) && window_trigger(app, TK_SPACE) ) starred = 1;
        }

        int stars = (dbgames[i] >> 8);
        int flags = (dbgames[i] & 0x7F);
        int color =
            flags == 0 ? '\x7' : // untested
            flags == 1 ? '\x2' : // fail
            flags == 2 ? '\x6' : // warn
                         '\x4' ; // good

        const char *title = sep+1;

        char wildcard[128] = {0};
        if( filter && filter[0] && snprintf(wildcard, 128, "*%s*", filter) ) {
            ui_alpha = 128;
            if( strmatchi(title, wildcard) ) ui_alpha = 255;
        }

        ui_at(ui, 1*11 + 3, (2+y++) * 11 + 2 );

        sprintf(buffer, "%3d.%s", i+1, i == selected ? ">":" ");
        ui_label(buffer);

#if 0
        ui_y--;
        if( is_file && ui_click("-Played-", ZXFlashFlag ? "":"▏▏\b\b\b\b" ) );
        ui_y++;
#endif

        if( is_file && ui_click("-Toggle bookmark-", va("%c\f", "\x10\x12"[!!stars])) )
            starred = 1;

        if( is_file && ui_click("-Toggle compatibility flags-\n\2fail\7, \6warn\7, \4good", va("%c%s", color, flags == 0 || flags == 3 ? "✓":"╳")) ) // "":""
            flagged = 1;

        sprintf(buffer, "%s%c%.*s\n",
            is_dir ? "\6" FOLDER_STR "\f" : " ", color, (int)(strlen(title) - is_dir), title );

        if( ui_click(NULL, buffer) ) selected = i, clicked = 1;

        if( is_file )
        if( starred || flagged ) {
            if( flagged ) flags = (flags+1) % 4;
            if( starred ) stars = stars != 'B' ? 'B' : 0;
            db_set(games[i], dbgames[i] = flags + (stars << 8));
        }
    }

    // issue browser
    // if( window_trigger(app, TK_LEFT)  ) { for(--up; (selected+up) >= 0 && (dbgames[selected+up]&0xFF) <= 1; --up ) ; }
    // if( window_trigger(app, TK_RIGHT) ) { for(++up; (selected+up) < numgames && (dbgames[selected+up]&0xFF) <= 1; ++up ) ; }

    if( clicked ) {
        return games[selected];
    }

    return NULL;
}



char* game_browser_v2() {
    // decay to local file browser if no ZXDB is present
    if( !zxdb_loaded() ) return ZX_BROWSER = 1, NULL;

#if 0
    if (!numgames) return 0;
    if( scroll < 0 ) scroll = 0;
#endif

    int selection[2] = {0};

    // handle input
    struct mouse m = mouse();
    int up = window_keyrepeat(app, TK_UP);
    int down = window_keyrepeat(app, TK_DOWN);
    int left = window_keyrepeat(app, TK_LEFT);
    int right = window_keyrepeat(app, TK_RIGHT);
    int page_up = window_keyrepeat(app,TK_PAGEUP);
    int page_down = window_keyrepeat(app,TK_PAGEDN);

    // constants

    const int LINE_HEIGHT = 11;
    const int UPPER_SPACING = 2;
    const int BOTTOM_SPACING = (DEV ? 5 : 3) * LINE_HEIGHT;

    // vars

    static int page = 0;
    static int thumbnails = 0; // 0 text, 3 (3x3), 6 (6x6), 12 (12x12)

    static VAL **list = 0;
    static int list_num = 0;

    // background (text mode only)

    static rgba *background_texture = 0;
    if( thumbnails == 0 )
    if( background_texture ) {
        int factor = 0, f256 = 256/(1<<factor), f192 = 192/(1<<factor);
        for(int i = 0, x = _32, y = _24; i < f192; ++i) {
            memcpy(&ui->pix[x+(y+i)*_320], background_texture + (0+i*f256), f256*4);
        }
    }

    // upper tabs

    ui_at(ui, 11-4-2, UPPER_SPACING);

//  static const char *tab = 0;
    static const char *tabs = "\x17#ABCDEFGHIJKLMNOPQRSTUVWXYZ\x12\x18"; // "⧖"

    do_once tab = tabs+2; // 'A'

    for(int i = 0; tabs[i]; ++i) {
        if( (ui_at(ui, ui_x+4+20*(i==0), ui_y), ui_button(NULL, va("%c%c", (tab && tabs[i] == *tab) ? 5 : 7, tabs[i])) )) {
            if( ui_hover ) {
                /**/ if(tabs[i] == '\x17') ui_notify( "-Browse local folder-\nClick again to change folder" );
                else if(tabs[i] == '\x12') ui_notify( "-List Bookmarks-\nClick again to change thumbnails view" );
                else if(tabs[i] == '\x18') ui_notify( "-Search-" );
                else if(tabs[i] ==    '#') ui_notify( "-List other games-\nClick again to change thumbnails view" );
                else                       ui_notify( va("-List %c games-\nClick again to change thumbnails view", tabs[i]) );
            }
            if( ui_click ) {
                if( tab == (tabs+i) ) {
                    // reclick
                    if( *tab == '\x17' ) {
                        extern int cmdkey;
                        extern const char *cmdarg;
                        cmdkey = 'SCAN';
                        cmdarg = 0; // ZX_FOLDER;
                    }
                    else if( *tab == '\x18' ) {
                        const char *search = prompt("Game search", ""/*"Either \"#zxdb-id\", \"*text*search*\", or \"/file.ext\" path"*/, "");
                        search_query_v1( search );
                    }
                    else {
                        int next[] = { [0]=3,[3]=6,[6]=12,[12]=0 };
                        thumbnails = next[thumbnails];
                    }
                }
                tab = tabs + i;
            }
        }
    }

#if 0
    if( left )  if(!tab) tab = tabs; else if(*tab-- == '#') tab = tabs + 29;
    if( right ) if(!tab) tab = tabs; else if(*tab++ == 'Z') tab = tabs +  3;
#else
    const char *first = strchr(tabs,numgames ? '\x17' : '#'), *last = strrchr(tabs,'\x12');

    if(!tab) tab = tabs;
    tab += right - left;
    if( tab < first || tab > last ) tab = left ? last : right ? first : tab;
#endif

    static const char *prev = 0;
    if( tab && prev != tab ) {

        selected = scroll = 0; // reset keyboard scroller

        if( *tab == '\x17' ) {
            extern int cmdkey;
            extern const char *cmdarg;

            do_once cmdkey = 'SCAN', cmdarg = ZX_FOLDER;

            //ZX_BROWSER = 1; // decay to file browser

            prev = tab;
            return NULL;
            //tab = 0;
            //prev = 0;
            //list = 0;
            //list_num = 0;
            //return NULL;
        }
        else
        if( *tab == '\x18' ) {
            const char *search = prompt("Game search", ""/*"Either \"#zxdb-id\", \"*text*search*\", or \"/file.ext\" path"*/, "");

            if( search && search[0] ) {
                search_query_v1( search );

                prev = tab;
                return NULL;
            } else {
                tab = prev;
                if(!tab) tab = prev = tabs + 2; // 'A'
                return NULL;
            }

            //list = 0;
            //list_num = 0;
            //return NULL;
        }
        else {
            page = 0;
        }

        // search & sort
        free(list), list = 0;
        if( tab && *tab != '\x12' ) {
            list = map_multifind(&zxdb2, va("%c*", *tab == '#' ? '?' : *tab), &list_num);
        }
        else {
            // bookmarks
            list_num = 0;
            list = realloc(list, 65535 * sizeof(VAL*));
            for( int i = 0; i < 65535; ++i) {
                if( cache_get(i) & 0x0f ) {
                    list[ list_num++ ] = map_find(&zxdb2, va("#%d", i));
                }
            }
            // list = realloc(list, list_num * sizeof(VAL*));
        }
        if( list_num ) qsort(list, list_num, sizeof(VAL*), zxdb_compare_by_name);

        // remove dupes (like aliases)
        for( int i = 1; i < list_num; ++i ) {
            if( *(char**)list[i-1] == *(char**)list[i] ) {
                memmove(list + i - 1, list + i, ( list_num - i ) * sizeof(list[0]));
                --list_num;
            }
        }

        // exclude XXX games
        // exclude For(S)ale,(N)everReleased,Dupes(*),MIA(?) [include (A)vailable,(R)ecovered,(D)enied games]
        // exclude demos 72..78
        for( int i = 0; i < list_num; ++i ) {
            char *zx_id = (char*)*list[i];
            char *years = strchr(zx_id, '|')+1; int zx_id_len = years-zx_id-1;
            char *title = strchr(years, '|')+1; int years_len = title-years-1;
            char *alias = strchr(title, '|')+1; int title_len = alias-title-1;
            char *brand = strchr(alias, '|')+1; int alias_len = brand-alias-1;
            char *avail = strchr(brand, '|')+1; int brand_len = avail-brand-1;
            char *score = strchr(avail, '|')+1; int avail_len = score-avail-1;
            char *genre = strchr(score, '|')+1; int score_len = genre-score-1;
            char *tags_ = strchr(genre, '|')+1; int genre_len = tags_-genre-1;

            if( avail[1] == 'X' || !strchr("ARD", avail[0]) || atoi(genre) >= 72 ) {
                memmove(list + i, list + i + 1, ( list_num - i - 1 ) * sizeof(list[0]));
                --list_num;
                --i;
            }
        }

        prev = tab;
    }

    // main content

    double progress_x = _320 * worker_progress(), progress_y = ui_y + LINE_HEIGHT;
    tigrLine(ui, 0, progress_y+1, progress_x, progress_y+1, ui_00);
    tigrLine(ui, 0, progress_y, progress_x, progress_y, ui_ff);

    if( !tab ) return NULL;

    if( *tab == '\x17' ) return game_browser_v1(); // NULL;
    if( *tab == '\x18' ) return search_results_v1();

    ui_at(ui, 0, UPPER_SPACING+2*LINE_HEIGHT);

    int ENTRIES_PER_PAGE = 25; // (_240-ui_y-BOTTOM_SPACING)/LINE_HEIGHT;

    if( thumbnails == 0 ) ENTRIES_PER_PAGE = 25;
    if( thumbnails == 3 ) ENTRIES_PER_PAGE = 3*3;
    if( thumbnails == 6 ) ENTRIES_PER_PAGE = 6*6;
    if( thumbnails == 12 ) ENTRIES_PER_PAGE = 12*12;

#if 0
    int NUM_PAGES = list_num / ENTRIES_PER_PAGE;
    int trailing_page = list_num % ENTRIES_PER_PAGE;
    NUM_PAGES -= NUM_PAGES && !trailing_page;

    if( page > NUM_PAGES ) page = NUM_PAGES - 1;
    if( page < 0 ) page = 0;

    if( up || page_up )     if(--page < 0) page = 0;
    if( down || page_down ) if(++page >= NUM_PAGES) page = NUM_PAGES;

    if( window_pressed(app, TK_HOME) ) page = 0;
    if( window_pressed(app, TK_END)  ) page = NUM_PAGES;
#endif

    int chosen = game_browser_keyboard(ENTRIES_PER_PAGE, list_num);

    static byte frame4 = 0; frame4 = (frame4 + 1) & 3; // 4-frame anim
    static byte frame8 = 0; frame8 = (frame8 + 1) & 7; // 8-frame anim

    if( 0 && thumbnails ) {
        scroll = scroll - (sqrt(ENTRIES_PER_PAGE) - (scroll % (int)sqrt(ENTRIES_PER_PAGE)));
        if( scroll < 0 ) scroll = 0;
    }

    if( list )
    for( int j = 0, cnt = 0, len; j < ENTRIES_PER_PAGE; ++j, ++cnt ) {
        int i = scroll + j;
        if( i < 0 ) continue;
        if( i >= list_num ) continue;

        char *zx_id = (char*)*list[i];
        char *years = strchr(zx_id, '|')+1; int zx_id_len = years-zx_id-1;
        char *title = strchr(years, '|')+1; int years_len = title-years-1;
        char *alias = strchr(title, '|')+1; int title_len = alias-title-1;
        char *brand = strchr(alias, '|')+1; int alias_len = brand-alias-1;
        char *avail = strchr(brand, '|')+1; int brand_len = avail-brand-1;
        char *score = strchr(avail, '|')+1; int avail_len = score-avail-1;
        char *genre = strchr(score, '|')+1; int score_len = genre-score-1;
        char *tags_ = strchr(genre, '|')+1; int genre_len = tags_-genre-1;

        // replace title if alias is what we're looking for
        if( *tab == '#' ) {
        if( *alias != '|' && (isdigit(*alias) || ispunct(*alias)) ) title = alias, title_len = alias_len;
        } else {
        if( title[0] != *tab && alias[0] == *tab ) title = alias, title_len = alias_len;
        }

        // also display alias right after any matching IDs are found twice
        // ie, gremlins 2: la nueva generacion (#2139) and gremlins 2: the new batch (#2139)
        bool use_alt = i && atoi((char*)*list[i]) == atoi((char*)*list[i-1]);
        if( use_alt ) {
            title = alias; title_len = alias_len;
        }

        // replace year if title was never released
        if( years[0] == '9' ) years = "?", years_len = 1; // "9999"

        // replace brand if no brand is given. use 1st author if possible
        if( brand[0] == '|' ) {
            char *next = strchr(zx_id, '\n');
            if( next && next[1] == '@' ) { // x3 skips: '\n' + '@' + 'R'ole
                brand = next+1+1+1, brand_len = strcspn(brand, "@\r\n");
            }
        }

        // stars, user-score
        const char *stars[] = {
            /*"\2"*/"\f\x10\f\x10\f\x10", // 0 0 0
            /*"\2"*/"\f\x11\f\x10\f\x10", // 0 0 1
            /*"\2"*/"\f\x12\f\x10\f\x10", // 0 1 0
            /*"\2"*/"\f\x12\f\x11\f\x10", // 0 1 1
            /*"\2"*/"\f\x12\f\x12\f\x10", // 1 0 0
            /*"\2"*/"\f\x12\f\x12\f\x11", // 1 0 1
            /*"\2"*/"\f\x12\f\x12\f\x12", // 1 1 0
            /*"\2"*/"\f\x12\f\x12\f\x12", // 1 1 1
        };
        static char colors[] = "\7\2\6\4";

        extern zxdb ZXDB;
        int loaded = ZXDB.ids[0] && atoi(ZXDB.ids[0]) == atoi(zx_id);
        if( loaded ) selection[0] = ui_x, selection[1] = ui_y;
        colors[0] = loaded && ZXFlashFlag ? '\5' : '\7';

        int dbid = atoi(zx_id);
        int vars = cache_get(dbid);
        int star = (vars >> 0) & 0x0f; assert(star <= 3);
        int flag = (vars >> 4) & 0x0f; assert(flag <= 3);

        // build title and clean it up
        char full_title[64];
        snprintf(full_title, sizeof(full_title), " %.*s (%.*s)(%.*s)", title_len, title, years_len, years, brand_len, brand);
        for( int i = 1; full_title[i]; ++i )
            if( i == 1 || full_title[i-1] == '.' )
                full_title[i] = toupper(full_title[i]);

        full_title[0] = colors[flag];

        int clicked = 0, flagged = 0, starred = 0;
        int iterated = selected == i;
        int dimmed = 0;

        char wildcard[128] = {0};
        if( filter && filter[0] && snprintf(wildcard, 128, "*%s*", filter) ) {
            dimmed = !strmatchi(full_title, wildcard);
            ui_alpha = !thumbnails && dimmed ? 64 : 255;
        }

        if( !thumbnails ) {

            ui_label(va("%c %c%3d.%s", colors[0], loaded ? '*':' ', i+1, i==selected ? ">":" "));

            if( ui_click("-Toggle bookmark-", va("%c\f", "\x10\x12"[!!star])) )
                starred = 1;

            if( ui_click("-Toggle compatibility flags-\n\2fail\7, \6warn\7, \4good", va("%c%s", colors[flag], flag == 0 || flag == 3 ? "✓":"╳")) ) // "":""
                flagged = 1;

            ui_label(" ");

            ui_monospaced = 0;
            if( ui_button(NULL, full_title) ) {
                
                if( ui_hover ) {
                    if( background_texture ) free(background_texture), background_texture = 0;

                    const byte *data = zxdb_screen_async(va("#%.*s", zx_id_len, zx_id), &len, 0);
                    if( data ) {
                        int ix,iy,in;
                        rgba *bitmap = (rgba*)stbi_load_from_memory(data, len, &ix, &iy, &in, 4);
                        if( bitmap ) {
                            background_texture = ui_resize(bitmap, ix, iy, 256/1, 192/1, 1, 0);
                            stbi_image_free(bitmap);
                        } else {
                            background_texture = thumbnail(data, len, 1, ZXFlashFlag);
                        }
                    }
                }

                if( ui_click ) {
                    clicked = 1;
                }
            }

            ui_button(NULL, "\n");
            ui_y--; // compact
        }
        else {
            int t = thumbnails;
            int w = _320/t, h = (_240-16)/t;
            int x = (cnt % t) * w, y = 16 + (cnt / t) * h;
            int has_thumb = 0;

            int factor = t == 3 ? 1 : t == 6 ? 2 : 3;
            const char *data = zxdb_screen_async(va("#%.*s", zx_id_len, zx_id), &len, factor);
            if( data && len ) {
                // blit
                const rgba *bitmap = (rgba*)data;
                int f256 = 256/(1<<factor), f192 = 192/(1<<factor);
                for(int i = 0; i < f192; ++i) {
                    memcpy(&ui->pix[x+(y+i)*_320], bitmap + (0+i*f256), f256*4);
                }

                has_thumb = 1;
            }

            if( dimmed && filter && filter[0] )
            for( int iy = y, iyend = y+h; iy < iyend; ++iy )
            for( TPixel *ix = &ui->pix[ x + iy * _320 ], *ixend = ix+w; ix < ixend; ++ix ) {
                ix->rgba = ( ix->rgba & 0x00f8f8f8 ) >> 3 | 0xff000000;
            }

            int hovered = m.x >= x && m.x < (x+w) && m.y >= y && m.y < (y+h);
            if( hovered || iterated || loaded ) {
                unsigned bak = ui_colors[1];
                if( hovered  ) ui_colors[1] = RGB(255,255,255);
                if( iterated ) ui_colors[1] = RGB(255,255,255);
                if( loaded   ) ui_colors[1] = ZXFlashFlag ? RGB(0,255,255) : RGB(255,255,255);
                ui_rect(ui, x,y, x+w-1,y+h-1);
                ui_colors[1] = bak;
            }
            if( hovered || iterated ) {
                ui_at(ui, x+2, y+2);

                if( ui_click(NULL, va("%c\f", "\x10\x12"[!!star])) )
                    starred = 1;

                if( ui_click(NULL, va("%c%s", colors[flag], flag == 0 || flag == 3 ? "✓":"╳")) ) // "":""
                    flagged = 1;

                if( hovered ) {
                    if( m.x <= (x+10) && m.y <= (y+11) ) {
                        ui_notify("-Toggle bookmark-");
                    }
                    else
                    if( m.x <= (x+10+10-4) && m.y <= (y+11) ) {
                        ui_notify("-Toggle compatibility flags-\n\2fail\7, \6warn\7, \4good");
                    }
                    else {
                        mouse_cursor(2);
                        ui_notify(full_title);
                        clicked = m.lb;
                    }
                }
            }
            else {
                ui_at(ui, x, y);

                if( !has_thumb ) {
                    const char *anims8[] = {
                        "","","","",
                        "","","","",
                    };
                    const char *anims4[] = {
                        "","",
                        "","",
                    };
                    const char *id = va("%x\n%s", atoi(zx_id), anims4[ (atoi(zx_id) + frame4) & 3 ] );
                    if( ui_click(id, id) ) clicked = 1;
                }
            }
        }

        if( !num_options )
        if( selected == i ) {
            if(!tigrKeyHeld(app, TK_SHIFT) && tigrKeyDown(app, TK_SPACE) ) starred = 1;
            if( tigrKeyHeld(app, TK_SHIFT) && tigrKeyDown(app, TK_SPACE) ) flagged = 1;
        }

        if( flagged || starred ) {
            if( starred ) star = !star;
            if( flagged ) flag = (flag + 1) % 4;
            cache_set(dbid, (vars & 0xff00) | (flag << 4) | star);
        }
                    
        if( clicked || (chosen >= 0 && chosen == i) ) {
            selected = i;

            active = 0;
#if 0
//          tab = 0;
            prev = 0;
            list = 0;
            list_num = 0;
#endif

            extern int cmdkey;
            extern const char *cmdarg;
            cmdkey = 'ZXDB', cmdarg = va("#%.*s", zx_id_len, zx_id);

            //return NULL;
        }
    }

    return NULL;
}
