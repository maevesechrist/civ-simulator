////////////////////////////////////////////////////////
// James Tobat
//
// File name: simulate.cpp
// Description: Implementation file for the simulate class
// Date: 12/11/13
//
#include "simulate.h"
// Simulation construction:
// Requires the mapsize to be used in advance.
// Constructor alone does not setup simulation
// as it requires the runSim method to set up/run the rest
simulate::simulate(int mapsizeX, int mapsizeY)
{
	currentTurn = 0; // Turn 0 = setup
	simfail = 0;
	numPlayers = 2;
	output.open("action_list.txt"); // Output of all simulation actions
					// to be used in another part of the program
	// Prints an error if the action list cannot be opened
	// this means the simulation has failed
	if(!output.is_open())
	{
		numTurns = 0;
		simfail = 1;
		printError(1);
	}
	// Prints an error if an improper mapsize is given
	// which means the simulation has failed
	else if(mapsizeX <= 0 || mapsizeY <= 0)
	{
		numTurns = 0;
		simfail=1;
		printError(2);
	}
	else
	{
		// Establishes a 2D vector to hold the map
		mapX = mapsizeX;
		mapY = mapsizeY;
		map.resize(mapX);
		
		for(int i = 0; i<mapX; i++)
		{
			map[i].resize(mapY);
		}
	}
}

// Reads in the simulation map from a map file
void simulate::populateMap(ifstream& mapFile)
{
	char mapChar;
	string read;
	bool end = false;
	int x = 0;
	int y = 0;
	// Reads until the end of the file or
	// when it goes beyond the number of rows
	// specified with mapX, whatever comes first.
	while(!mapFile.eof() && x!=mapX)
	{
		getline(mapFile,read);

		// Parses each line until up to the max
		// number of columns or mapY.
		// Sets up each piece of the map with
		// a terrain character which is read
		// from this file, as well as, no starting army
		// and cities/roads, this will be done later.
		for(int i = 0; i < mapY; i++)
		{
			mapChar = read[i];
			map[x][i].terrain = mapChar;
			map[x][i].unit = false;
			map[x][i].city = 0;
		}
		x++;
	}
}

// Connects all the parts of the simulation as well as completes set up.
void simulate::runSim()
{
	setup(); // Places starting cities on map
	int color; // Represents the current player
		   // which is used to distinguish who's turn it is
	p1armies = 0; // Number of armies generated by player 1
	p2armies = 0; // Number of armies generated by player 2

	// Runs through all the turns of the simulation which is specified in the config
	// file.
	for(int i = 0; i<numTurns; i++)
	{
		if(simfail)
		{
			break;
		}
		currentTurn++;
		output<<i+1<<endl;

		// Determines who's turn it is
		// then allows each player to act one after the other
		for(int j = 0; j<numPlayers; j++)
		{
			if(j == 0)
			{
				color = p1Color;
			}
			else
			{
				color = p2Color;
			}
			simCities(color);
			simArmies(color);
		}
	}
}

// Simulates the actions of each army.
// Armies will try and destroy other armies
// first then try and take over cities/roads in that order.
// If they do none of those things, they will move to a new location.
// Armies can move through all terrain except for mountains and ocean.
// The player input determines which player is acting and which one is not.
void simulate::simArmies(int player)
{
	int x,y;
	int x1,y1;
	vector <coord> adjacent;
	int enemy;
	bool destr = false;
	int random;
	
	// Will produce an action for each army that a player has
	for(unsigned int i = 0; i<army.size();i++)
	{
		// If an enemy army has been destroyed
		// it will end the current player's turn immediatly
		if(destr)
		{
			break;
		}
		// Checks to see if there is an enemy army adjacent.
		if(army[i].color == player)
		{
			x = army[i].x;
			y = army[i].y;
			adjacent.erase(adjacent.begin(),adjacent.end());
			findAdjacent(x,y,adjacent);
			// Will destroy an adjacent enemy army
			for(unsigned int j = 0; j<adjacent.size();j++)
			{
				x1 = adjacent[j].x;
				y1 = adjacent[j].y;
				enemy = findArmy(x1,y1);
				if(enemy >= 0)
				{
					if(army[enemy].color != player)
					{
						destroy(2,x1,y1);
						destr = true;
						break;
					}
				}
			}
			
			// Checks to see if an adjacent space is an enemy city, if it is,
			// it will take over the city.
			for(unsigned int k = 0; k<adjacent.size();k++)
			{
				if(destr)
				{
					break;
				}

				x1 = adjacent[k].x;
				y1 = adjacent[k].y;
				enemy = findCity(x1,y1);
				if(enemy >= 0)
				{
					if(city[enemy].color != player)
					{
						color(1,x1,y1,player);
						//destr = true;
						break;
					}
				}
			}

			// Checks to see if an adjacent space is an enemy road, if it is,
			// it will take over the road.
			for(unsigned int k = 0; k<adjacent.size();k++)
			{
				if(destr)
				{
					break;
				}

				x1 = adjacent[k].x;
				y1 = adjacent[k].y;
				enemy = findRoad(x1,y1);
				if(enemy >= 0)
				{
					if(road[enemy].color != player)
					{
						color(2,x1,y1,player);
						destr = true;
						break;
					}
				}
			}

			// If the army has done nothing this turn, it will move to a new location
			// that does not have an enemy city/road/unit on it and that is not a mountain or ocean.
			for(unsigned int k = 0; k<adjacent.size();k++)
			{
				if(destr)
				{
					break;
				}
				random = rand() % adjacent.size();
				x1 = adjacent[random].x;
				y1 = adjacent[random].y;

				if(map[x1][y1].unit == false && (map[x1][y1].city == 0 || map[x1][y1].city == 2))
				{
					moveUnit(x,y,x1,y1);
					break;
				}
			}
		}
	}
}

// Simulates the actions of each city/road that has been created.
// Cities will create armies up until the maximum every 5 turns.
// Cities will create roads every three turns.
// Roads expand into all adjacent spaces (no mountains,no oceans, no existing city/road).
// Once roads have expanded into all available adjacent spaces, then each road piece will start
// to have connecting branches of their own.
// The player input determines which player is acting and which one is not.
void simulate::simCities(int player)
{
	int x,y;
	int x1,y1;
	int cityl;
	int built = 0;
	vector <coord> adjacent;
	// Performs actions for each city in the simulation.
	for(unsigned int i = 0;i<city.size();i++)
	{
		// Checks to see if the city belongs to the current player
		if(city[i].color == player)
		{
			x = city[i].x;
			y = city[i].y;

			// Creates an army in the city if one isn't already there
			// and if 5 turns have passed.
			if(map[x][y].unit == false && currentTurn % 5 == 0)
			{
				if(player == p1Color && p1armies<maxArmies)
				{
					create(3,x,y,player);
					p1armies++;
				}
				else if(player == p2Color && p2armies<maxArmies)
				{
					create(3,x,y,player);
					p2armies++;
				}
			}
			// Expands the roads
			if(currentTurn % 3 == 0)
			{
				adjacent.erase(adjacent.begin(),adjacent.end());
				findAdjacent(x,y,adjacent);
				for(unsigned int k = 0; k<adjacent.size();k++)
				{
					x1 = adjacent[k].x;
					y1 = adjacent[k].y;
					cityl = map[x1][y1].city;
					if((map[x1][y1].unit != true) && cityl == 0)
					{
						x1 = adjacent[k].x;
						y1 = adjacent[k].y;
						create(2,x1,y1,player);
						built++;
						break;
					}
				}
			}
		}
	}
	
	// If no roads have been built adjacent to a city, then it checks to see if it
	// can expand a road branch.
	if(built == 0 && currentTurn % 3 == 0)
	{
		for(unsigned int l = 0; l<road.size();l++)
		{
			if(built > 0)
			{
				break;
			}

			if(road[l].color == player)
			{
				x = road[l].x;
				y = road[l].y;
				adjacent.erase(adjacent.begin(),adjacent.end());
				findAdjacent(x,y,adjacent);
				for(unsigned int k = 0; k<adjacent.size();k++)
				{
					x1 = adjacent[k].x;
					y1 = adjacent[k].y;
					cityl = map[x1][y1].city;
					if((map[x1][y1].unit != true) && cityl == 0)
					{
						create(2,x1,y1,player);
						built++;
						break;
					}
				}
			}
		}
	}
}

// Finds all adjacent spaces to a unit (road/city/army).
// Ignores mountains and oceans.
// Adds suitable locations to the provided vector.
void simulate::findAdjacent(int x, int y,vector<coord>&adj)
{
	// Special cases
	bool trCorner = false; // top right corner case
	bool tlCorner = false; // top left corner case
	bool brCorner = false; // bottom right corner case	
	bool blCorner = false; // bottom left corner case
	bool rEdge = false; // right edge case
	bool lEdge = false; // left edge case
	bool tEdge = false; // top edge case
	bool bEdge = false; // bottom edge case
	coord temp;

	// Determines it the inputed coordinate is a special case 
	// or not.
	if(x == mapX-1 && y == mapY-1)
	{
		brCorner = true;
	}
	else if(x == 0 && y == 0)
	{
		tlCorner = true;
	}
	else if(x == mapX-1 && y == 0)
	{
		blCorner = true;
	}
	else if(x == 0 && y == mapY-1)
	{
		trCorner = true;
	}
	else if(x == mapX-1)
	{
		bEdge = true;
	}
	else if(x == 0)
	{
		tEdge = true;
	}
	else if(y == mapY - 1)
	{
		rEdge = true;
	}
	else if(y == 0)
	{
		lEdge = true;
	}

	// Handles all cases, both special and not so special.
	// Generates adjacent space coordinates and stores them in the vector.
	// Minimum generated will be 2, while the max is 4.
	if(brCorner)
	{
		if(map[x-1][y].terrain != ocean && map[x-1][y].terrain != mountain)
		{
			temp.y = y;
			temp.x = x-1;
			adj.push_back(temp);
		}

		if(map[x][y-1].terrain != ocean && map[x][y-1].terrain != mountain)
		{
			temp.y = y-1;
			temp.x = x;
			adj.push_back(temp);
		}
	}
	else if(blCorner)
	{
		if(map[x-1][y].terrain != ocean && map[x-1][y].terrain != mountain)
		{
			temp.y = y;
			temp.x = x-1;
			adj.push_back(temp);
		}

		if(map[x][y+1].terrain != ocean && map[x][y+1].terrain != mountain)
		{
			temp.y = y+1;
			temp.x = x;
			adj.push_back(temp);
		}
	}
	else if(trCorner)
	{
		if(map[x][y-1].terrain != ocean && map[x][y-1].terrain != mountain)
		{
			temp.y = y-1;
			temp.x = x;
			adj.push_back(temp);
		}

		if(map[x+1][y].terrain != ocean && map[x+1][y].terrain != mountain)
		{
			temp.y = y;
			temp.x = x+1;
			adj.push_back(temp);
		}
	}
	else if(tlCorner)
	{
		if(map[x][y+1].terrain != ocean && map[x][y+1].terrain != mountain)
		{
			temp.y = y+1;
			temp.x = x;
			adj.push_back(temp);
		}

		if(map[x+1][y].terrain != ocean && map[x+1][y].terrain != mountain)
		{
			temp.y = y;
			temp.x = x+1;
			adj.push_back(temp);
		}
	}
	else if(bEdge)
	{
		if(map[x][y+1].terrain != ocean && map[x][y+1].terrain != mountain)
		{
			temp.y = y+1;
			temp.x = x;
			adj.push_back(temp);
		}

		if(map[x][y-1].terrain != ocean && map[x][y-1].terrain != mountain)
		{
			temp.y = y-1;
			temp.x = x;
			adj.push_back(temp);
		}

		if(map[x-1][y].terrain != ocean && map[x-1][y].terrain != mountain)
		{
			temp.y = y;
			temp.x = x-1;
			adj.push_back(temp);
		}
	}
	else if(tEdge)
	{
		if(map[x][y+1].terrain != ocean && map[x][y+1].terrain != mountain)
		{
			temp.y = y+1;
			temp.x = x;
			adj.push_back(temp);
		}

		if(map[x][y-1].terrain != ocean && map[x][y-1].terrain != mountain)
		{
			temp.y = y-1;
			temp.x = x;
			adj.push_back(temp);
		}

		if(map[x+1][y].terrain != ocean && map[x+1][y].terrain != mountain)
		{
			temp.y = y;
			temp.x = x+1;
			adj.push_back(temp);
		}
	}
	else if(rEdge)
	{
		if(map[x-1][y].terrain != ocean && map[x-1][y].terrain != mountain)
		{
			temp.y = y;
			temp.x = x-1;
			adj.push_back(temp);
		}

		if(map[x][y-1].terrain != ocean && map[x][y-1].terrain != mountain)
		{
			temp.y = y-1;
			temp.x = x;
			adj.push_back(temp);
		}

		if(map[x+1][y].terrain != ocean && map[x+1][y].terrain != mountain)
		{
			temp.y = y;
			temp.x = x+1;
			adj.push_back(temp);
		}
	}
	else if(lEdge)
	{
		if(map[x-1][y].terrain != ocean && map[x-1][y].terrain != mountain)
		{
			temp.y = y;
			temp.x = x-1;
			adj.push_back(temp);
		}

		if(map[x][y+1].terrain != ocean && map[x][y+1].terrain != mountain)
		{
			temp.y = y+1;
			temp.x = x;
			adj.push_back(temp);
		}

		if(map[x+1][y].terrain != ocean && map[x+1][y].terrain != mountain)
		{
			temp.y = y;
			temp.x = x+1;
			adj.push_back(temp);
		}
	}
	else
	{
		if(map[x-1][y].terrain != ocean && map[x-1][y].terrain != mountain)
		{
			temp.y = y;
			temp.x = x-1;
			adj.push_back(temp);
		}

		if(map[x][y+1].terrain != ocean && map[x][y+1].terrain != mountain)
		{
			temp.y = y+1;
			temp.x = x;
			adj.push_back(temp);
		}

		if(map[x][y-1].terrain != ocean && map[x][y-1].terrain != mountain)
		{
			temp.y = y-1;
			temp.x = x;
			adj.push_back(temp);
		}

		if(map[x+1][y].terrain != ocean && map[x+1][y].terrain != mountain)
		{
			temp.y = y;
			temp.x = x+1;
			adj.push_back(temp);
		}
	}
}
// Parses the config file for needed variables in class.
void simulate::parseConfig(ifstream& config)
{
	string read;
	size_t pos;
	int position = 0;
	int start,end;
	const int numParams = 10;
	// All paramters, will look for this exact string in the file.
	string params[numParams] = {"turns","maximum_armies",
						"cities_per_player",
						"player1_color",
						"player2_color",
						"plains_character",
						"mountain_character",
						"forest_character",
						"ocean_character",
						"river_character"};
	// Reads each line in the file
	while(!config.eof())
	{
		getline(config,read);
		string temp;
		stringstream s;

		for(int i = 0; i<numParams; i++)
		{
			// If a parameter has been found
			// , the program will determine
			// the correct constant to store it in.
			pos = read.find(params[i]);
			end = 0;
			if(pos != string::npos)
			{
				position = 0;
				pos = read.find_first_of("'");
				start = pos;
				switch(i)
				{
					case(0): 
					pos = read.find_last_of("'");
					end = pos;
					temp = read.substr(start+1,end-1);
					s.str(temp);
					s>>numTurns;
					break;

					case(1):
					pos = read.find_last_of("'");
					end = pos;
					temp = read.substr(start+1,end-1);
					s.str(temp);
					s>>maxArmies;
					break;

					case(2):
					pos = read.find_last_of("'");
					end = pos;
					temp = read.substr(start+1,end-1);
					s.str(temp);
					s>>total_cities;
					break;

					case(3):
					pos = read.find_last_of("'");
					end = pos;
					temp = read.substr(start+1,end-1);
					s.str(temp);
					s>>p1Color;
					break;

					case(4):
					pos = read.find_last_of("'");
					end = pos;
					temp = read.substr(start+1,end-1);
					s.str(temp);
					s>>p2Color;
					break;

					case(5):
					plains = read[start+1];
					break;

					case(6):
					mountain = read[start+1];
					break;

					case(7):
					forest = read[start+1];
					break;

					case(8):
					ocean = read[start+1];
					break;

					case(9):
					river = read[start+1];
					break;
				}
			}
		}
	}
}

// Prints off errors encountered during the simulation to cerr
void simulate::printError(int errorNum)
{
	switch(errorNum)
	{
		case 1: cerr<<"simulate: Could not open the output file, simulation failed!\n";
		break;
		case 2: cerr<<"simulate: X and Y coordinates of map must be greater than 0, simulation failed!\n";
		break;
		default: cerr<<"simulate: unknown error, simulation failed!\n";
		break;
	}
}

// Places initial starting cities.
// This is established in the config file.
void simulate::setup()
{
	output<<"0"<<endl;
	char ter;
	coord temp;
	vector <coord> settle;
	int random,color;
	int x,y;
	// Stores all suitable starting locations in an array.
	// A suitable location will be a plains or forest.
	for(int i = 0;i<mapX;i++)
	{
		for(int j = 0;j<mapY;j++)
		{
			ter = map[i][j].terrain;
			if(ter == plains || ter == forest)
			{
				temp.x = i;
				temp.y = j;
				settle.push_back(temp);
			}
		}
	}
	
	if(settle.size() <= 0 || (settle.size() < (numPlayers*total_cities)))
	{
		cerr<<"simulate: map could not store all cities, simulation failed!\n";
		simfail = 1;
	}
	else
	{
		// Places x starting cities for each player.
		// x represents the total_cities variable.
		// Chooses the starting locations randomly
		for(int j = 0; j<numPlayers; j++)
		{
			if(j == 0)
			{
				color = p1Color;
			}
			else
			{
				color = p2Color;
			}

			for(int k = 0; k<total_cities; k++)
			{
				random = rand() % settle.size();	
				x = settle[random].x;
				y = settle[random].y;
				create(1,x,y,color);
				settle[random].x = settle.back().x;
				settle[random].y = settle.back().y;
				settle.pop_back();
			}
		}
	}
}

// Moves a unit from an old coordinate to a new one.
// The coordinates presented here are RC or row column coordinates.
// Will update all appropriate arrays as well as output a statement
// to the action list which states what occurred. 
void simulate::moveUnit(int x_old, int y_old, int x, int y)
{
	int arraySpot = findArmy(x_old,y_old);
	army[arraySpot].x = x;
	army[arraySpot].y = y;
	output<<"M "<<y_old<<" "<<x_old<<" "<<y<<" "<<x<<endl;
	map[x_old][y_old].unit = false;
	map[x][y].unit = true;
}

// Changes the color of an object at a given coordinate.
// Color represents an integer color code which belongs to
// either player 1 or 2. Object is either 1-3. 1 represents cities.
// 2 represents roads. 3 represents armies.
void simulate::color(int object, int x, int y, int color)
{
	int spot;
	int layer;
	switch(object)
	{
		case 1:
		layer = 1;
		spot = findCity(x,y);
		city[spot].color = color;
		break;
		case 2:
		layer = 1;
		spot = findRoad(x,y);
		road[spot].color = color;
		break;
		case 3:
		layer = 2;
		spot = findArmy(x,y);
		army[spot].color = color;
		break;
	}

	output<<"L "<<layer<<" "<<y<<" "<<x<<" "<<color<<endl;
}

// Destroys a unit (road/city/army) at a given location.
// Layer cann either be 1, 2, or 3 which represents cities, roads,
// or armies respectively.
// Will output this action to the action list to show what conspired
// during simulation.
void simulate::destroy(int layer, int x, int y)
{
	int spot;
	switch(layer)
	{
		case 1:
		if(map[x][y].city = 1)
		{
			spot = findCity(x,y);
			city[spot].x = city.back().x;
			city[spot].y = city.back().y;
			city[spot].color = city.back().color;
			city.pop_back();
		}
		else
		{
			spot = findRoad(x,y);
			road[spot].x = road.back().x;
			road[spot].y = road.back().y;
			road[spot].color = road.back().color;
			road.pop_back();
		}
		map[x][y].city = 0;
		break;

		case 2:
			spot = findArmy(x,y);
			army[spot].x = army.back().x;
			army[spot].y = army.back().y;
			army[spot].color = army.back().color;
			army.pop_back();
			map[x][y].unit = false;
		break;
	}
	output<<"D "<<layer<<" "<<y<<" "<<x<<endl;
}

// Will find an army unit at the given coordinate inside of the army vector.
// Returns the position of the vector where it is found otherwise returns -1.
int simulate::findArmy(int x, int y)
{
	int arraySpot = -1;

	for(unsigned int i = 0; i<army.size(); i++)
	{
		if(x == army[i].x && y == army[i].y)
		{
			arraySpot = i;
			break;
		}
	}

	return arraySpot;
}

// Will find a city unit at the given coordinate inside of the city vector.
// Returns the position of the vector where it is found otherwise returns -1.
int simulate::findCity(int x, int y)
{
	int arraySpot = -1;

	for(unsigned int i = 0; i<city.size(); i++)
	{
		if(x == city[i].x && y == city[i].y)
		{
			arraySpot = i;
			break;
		}
	}

	return arraySpot;
}

// Will find a road unit at the given coordinate inside of the road vector.
// Returns the position of the vector where it is found otherwise returns -1.
int simulate::findRoad(int x, int y)
{
	int arraySpot = -1;

	for(unsigned int i = 0; i<road.size(); i++)
	{
		if(x == road[i].x && y == road[i].y)
		{
			arraySpot = i;
			break;
		}
	}

	return arraySpot;
}

// Creates an object at the specified coordinate with the given color.
// Will output this action to the action list.
// Object is either 1-3 where 1 is a city, 2 is a road, and 3 is an army.
// This method updates all appopriate variables/vectors to make this happen.
void simulate::create(int object, int x, int y, int color)
{
	string type;
	simUnit temp;
	int layer;

	switch(object)
	{
		case 1:
		type = "city";
		map[x][y].city = 1;
		temp.color = color;
		temp.x = x;
		temp.y = y;
		layer = 1;
		city.push_back(temp);
		break;
		case 2:
		type = "road";
		map[x][y].city = 2;
		temp.color = color;
		temp.x = x;
		temp.y = y;
		layer = 1;
		road.push_back(temp);
		break;
		case 3:
		type = "army";
		map[x][y].unit = true;
		temp.color = color;
		temp.x = x;
		temp.y = y;
		layer = 2;
		army.push_back(temp);
		break;
	}

	output<<"C "<<layer<<" "<<y<<" "<<x<<" "<<color<<" "<<type<<endl;
}

