CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )

SET( PROJ_NAME      "gs" )
SET( PROJ_SOURCES   "src/main.cpp" )
SET( PROJ_INCLUDES  "inc" )
SET( PROJ_LIBRARIES "-lglfw3 -lopengl32" )

PROJECT( ${PROJ_NAME} )

INCLUDE_DIRECTORIES( ${PROJ_INCLUDES} )
ADD_EXECUTABLE( ${PROJ_NAME} ${PROJ_SOURCES} )
TARGET_LINK_LIBRARIES( ${PROJ_NAME} ${PROJ_LIBRARIES} )