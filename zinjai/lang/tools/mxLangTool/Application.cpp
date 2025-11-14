#include "Application.h"

#include "mxWindow.h"
using namespace std;

bool mxApplication::OnInit() {
	wxString ref = "spanish", edit="english";
	if (argc==2) edit = argv[1];
	else if (argc==3) ref = argv[1], edit = argv[2];
	new mxWindow(ref,edit);
	return true;
}
