import mars;

#define MARS_REPORT(proc) if(!proc.report()) return 1

int main(int argc, char** argv) {
    MARS_REPORT(mars::init());
    MARS_REPORT(mars::run());
    return mars::quit();
}
