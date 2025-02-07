// Copyright (C) 2022 Yanqi Pan <deadpoolmine@qq.com>
// Copyright (C) 2021 Jiansheng Qiu <632863936@qq.com>
//  
// HUNTER-REPO is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
// 
// HUNTER-REPO is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with HUNTER-REPO.  If not, see <http://www.gnu.org/licenses/>.



#include <iostream>
#include <string>

using namespace std;

int main() {
	double x;
	string unit;

	cin >> x;
	cin >> unit;
	if (unit == "MiB/s") {
		cout << x;
	} else if (unit == "GiB/s") {
		x = x * 1024;
    	cout << x;
	} else if (unit == "KiB/s") {
		x = x / 1024;
		cout << x;
	} else {
		cerr << "Unrecognized unit " << unit << endl;
		return 1;
	}

	return 0;
}
