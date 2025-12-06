import mars;

int main(int argc, char** argv) {
    if(!mars::init().report()) return 1;
    if(!mars::run().report()) return 1;
    return mars::quit();
}
