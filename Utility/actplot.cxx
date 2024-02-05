#include "TRint.h"

int main(int argc, char* argv[])
{
    TRint app {"actplot", &argc, argv, nullptr, 0, true};

    app.Run();

    return 0;
}
