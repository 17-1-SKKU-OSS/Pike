#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name="String Creation";

int k = 1000; /* variable to tune the time of the test */
int n;
string file = (string)enumerate(500*1024);

// Tests the string_magnitude, internal_findstring and memhash
// functions most (and malloc + memcpy).
//
void perform()
{
    int q;
    int z = 128;
    array ss;
    for( int i=0; i<k; i++ )
    {
        // This temporary is needed, otherwise the optimizer optimizes
        // away the division below.
        ss = file/(z+(k&(z-1)));
        q+=sizeof(ss);
        ss = ({});
    }
    n = q;
}

string present_n(int ntot,int nruns,float tseconds,float useconds,int memusage)
{
   return sprintf("%.0f/s",ntot/useconds);
}
