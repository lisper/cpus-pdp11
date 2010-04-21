cl -c -DWIN32 assemble.c
cl -c -DWIN32 assemble_aux.c
cl -c -DWIN32 assemble_globals.c 
cl -c -DWIN32 dumpobj.c
cl -c -DWIN32 object.c
cl -c -DWIN32 rept_irpc.c
cl -c -DWIN32 util.c
cl -c -DWIN32 extree.c
cl -c -DWIN32 macros.c
cl -c -DWIN32 parse.c 
cl -c -DWIN32 stream2.c    
cl -c -DWIN32 listing.c
cl -c -DWIN32 mlb.c 
cl -c -DWIN32 rad50.c
cl -c -DWIN32 symbols.c    
cl -c -DWIN32 macro11.c

link /out:m.exe  assemble.obj assemble_aux.obj assemble_globals.obj object.obj rept_irpc.obj util.obj extree.obj macros.obj parse.obj stream2.obj listing.obj mlb.obj rad50.obj symbols.obj macro11.obj
