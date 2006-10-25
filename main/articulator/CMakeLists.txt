PROJECT(articulator)

MESSAGE(STATUS "This is taken from GNUSPEECH")
MESSAGE(STATUS "see license at http://cvs.savannah.gnu.org/viewcvs/gnuspeech/?root=gnuspeech")

SET(YARP_DIR "$ENV{YARP_BUILD}" CACHE LOCATION "where is yarp?")

FIND_PACKAGE(YARP)

FILE(GLOB folder_source1 *.cc)
FILE(GLOB folder_source2 *.c)
FILE(GLOB folder_header *.h)
SOURCE_GROUP("Source Files" FILES ${folder_source1} ${folder_source2})
SOURCE_GROUP("Header Files" FILES ${folder_header})

#LINK_LIBRARIES(fftw3)

ADD_EXECUTABLE(articulator ${folder_source1} ${folder_source2} ${folder_header})

