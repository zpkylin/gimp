
#include <sys/time.h>
#include <unistd.h>

typedef struct
{
  struct timeval a;
  struct timeval b;
} TraceTimer;

void trace_enter   (char * func);
void trace_exit    (void); 
void trace_begin   (char * format, ...);
void trace_end     (void);
void trace_printf  (char * format, ...);

void trace_timer_init (TraceTimer * t);
void trace_timer_tick (TraceTimer * t, char * pre, char * post);
void trace_time_before ( TraceTimer *t );
void trace_time_after ( TraceTimer * t);
void print_elapsed (char *, TraceTimer *t ); 

struct _PixelArea;
struct _PixelRow;
struct _Canvas;

void 
print_area (
			struct _PixelArea *pa,
			int row,
			int how_many_rows
			);
void
pixelrow_print (
		 struct _PixelRow *row
		); 

void 
print_area_init(
                 struct _PixelArea * pa,
                 struct _Canvas * c,
                 int x,
                 int y,
                 int w,
                 int h,
                 int will_dirty,
		 char * name
		);
