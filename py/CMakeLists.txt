find_program(PYTHON "python")
## clam-prov.py is added by the root CMakeLists.txt
## install(PROGRAMS clam-prov.py  DESTINATION bin)
install(FILES stats.py DESTINATION bin)
