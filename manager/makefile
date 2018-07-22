OPENCV = $(shell pkg-config --cflags opencv) $(shell pkg-config --libs opencv)
LINK_ALL = $(OPENCV)

SRC = $(CAMERASHIFT) $(GUIDENCE) imgManager.cpp <yourfilename.cpp>
TAR = <yourfilename>

$(TAR):$(SRC)
    g++ -o $(TAR) $(SRC) $(LINK_ALL) -z execstack -fno-stack-protector -g

clean:
    rm $(OBJ) $(TAR)

%.o:%.cpp
    g++ -I $(LINK_ALL) -o $@ -c $<
