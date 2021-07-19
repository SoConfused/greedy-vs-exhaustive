////////////////////////////////////////////////////////////////////////////////
// maxtime.hh
//
// Compute the set of rides that maximizes the time spent at rides, within a given budget
// with the greedy method or exhaustive search.
//
///////////////////////////////////////////////////////////////////////////////


#pragma once


#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <queue>
#include <sstream>
#include <string>
#include <vector>
#include "timer.hh"


// One ride item available for purchase.
class RideItem
{
	//
	public:
		
		//
		RideItem
		(
			const std::string& description,
			double cost_dollars,
			double time_minutes
		)
			:
			_description(description),
			_cost_dollars(cost_dollars),
			_time_minutes(time_minutes)
		{
			assert(!description.empty());
			assert(cost_dollars > 0);
		}
		
		//
		const std::string& description() const { return _description; }
		double cost() const { return _cost_dollars; }
		double rideTime() const { return _time_minutes; }
	
	//
	private:
		
		// Human-readable description of the ride, e.g. "new enchanted world". Must be non-empty.
		std::string _description;
		
		// Ride cost, in units of dollars; Must be positive
		double _cost_dollars;
		
		// Ride time in minutes; most be non-negative.
		double _time_minutes;
};


// Alias for a vector of shared pointers to RideItem objects.
typedef std::vector<std::shared_ptr<RideItem>> RideVector;


// Load all the valid ride items from the CSV database
// Ride items that are missing fields, or have invalid values, are skipped.
// Returns nullptr on I/O error.
std::unique_ptr<RideVector> load_ride_database(const std::string& path)
{
	std::unique_ptr<RideVector> failure(nullptr);
	
	std::ifstream f(path);
	if (!f)
	{
		std::cout << "Failed to load ride database; Cannot open file: " << path << std::endl;
		return failure;
	}
	
	std::unique_ptr<RideVector> result(new RideVector);
	
	size_t line_number = 0;
	for (std::string line; std::getline(f, line); )
	{
		line_number++;
		
		// First line is a header row
		if ( line_number == 1 )
		{
			continue;
		}
		
		std::vector<std::string> fields;
		std::stringstream ss(line);
		
		for (std::string field; std::getline(ss, field, '^'); )
		{
			fields.push_back(field);
		}
		
		if (fields.size() != 3)
		{
			std::cout
				<< "Failed to load ride database: Invalid field count at line " << line_number << "; Want 3 but got " << fields.size() << std::endl
				<< "Line: " << line << std::endl
				;
			return failure;
		}
		
		std::string
			descr_field = fields[0],
			cost_dollars_field = fields[1],
			time_minutes_field = fields[2]
			;
		
		auto parse_dbl = [](const std::string& field, double& output)
		{
			std::stringstream ss(field);
			if ( ! ss )
			{
				return false;
			}
			
			ss >> output;
			
			return true;
		};
		
		std::string description(descr_field);
		double cost_dollars, time_minutes;
		if (
			parse_dbl(cost_dollars_field, cost_dollars)
			&& parse_dbl(time_minutes_field, time_minutes)
		)
		{
			result->push_back(
				std::shared_ptr<RideItem>(
					new RideItem(
						description,
						cost_dollars,
						time_minutes
					)
				)
			);
		}
	}

	f.close();
	
	return result;
}


// Convenience function to compute the total cost and time in an RideVector.
// Provide the RideVector as the first argument
// The next two arguments will return the cost and time back to the caller.
void sum_ride_vector
(
	const RideVector& rides,
	double& total_cost,
	double& total_time
)
{
	total_cost = total_time = 0;
	for (auto& ride : rides)
	{
		total_cost += ride->cost();
		total_time += ride->rideTime();
	}
}


// Convenience function to print out each RideItem in an RideVector,
// followed by the total kilocalories and protein in it.
void print_ride_vector(const RideVector& rides)
{
	std::cout << "*** ride Vector ***" << std::endl;
	
	if ( rides.size() == 0 )
	{
		std::cout << "[empty ride list]" << std::endl;
	}
	else
	{
		for (auto& ride : rides)
		{
			std::cout
				<< "Ye olde " << ride->description()
				<< " ==> "
				<< "Cost of " << ride->cost() << " dollars"
				<< "; time in minutes = " << ride->rideTime()
				<< std::endl
				;
		}
		
		double total_cost, total_time;
		sum_ride_vector(rides, total_cost, total_time);
		std::cout
			<< "> Grand total cost: " << total_cost << " dollars" << std::endl
			<< "> Grand total time: " << total_time
			<< std::endl
			;
	}
}


// Filter the vector source, i.e. create and return a new RideVector
// containing the subset of the ride items in source that match given
// criteria.
// This is intended to:
//	1) filter out ride with zero or negative time that are irrelevant to our optimization
//	2) limit the size of inputs to the exhaustive search algorithm since it will probably be slow.
//
// Each ride item that is included must have at minimum min_time and at most max_time.
//	(i.e., each included ride item's time must be between min_time and max_time (inclusive).
//	 
// In addition, the the vector includes only the first total_size ride items that match these criteria.
std::unique_ptr<RideVector> filter_ride_vector
(
	const RideVector& source,
	double min_time, 
	double max_time, 
	int total_size 
)
{
std::unique_ptr<RideVector> sortedVector(new RideVector);
for(int i = 0; i < source.size(); i++)
{
	if((*source[i]).rideTime() > 0 && ((*source[i]).rideTime() >= min_time && (*source[i]).rideTime() <= max_time) && (*sortedVector).size() < total_size)
	{
		(*sortedVector).push_back(source[i]);
	}
}
	return sortedVector;
}


// Compute the optimal set of ride items with a greedy algorithm.
// Specifically, among the ride items that fit within a total_cost dollar budget,
// choose the rides whose time is greatest.
// Repeat until no more ride items can be chosen, either because we've run out of ride items,
// or run out of dollars.
std::unique_ptr<RideVector> greedy_max_time
(
	const RideVector& rides,
	double total_cost
)
{
	std::unique_ptr<RideVector> todo(new RideVector(rides));
	std::unique_ptr<RideVector> result(new RideVector);
	double result_cost = 0;

	while(!(*todo).empty())
	{
		double maxTPC = 0;
		int indexMaxTPC = 0;
		
		// Find the ride item “a” in todo of maximum time per its cost
		for (int i = 0; i < (*todo).size(); i++)
		{
			if (i == 0)
			{
				maxTPC = (*todo)[i]->rideTime()/(*todo)[i]->cost();
			}
			else
			{
				double tpc = (*todo)[i]->rideTime() / (*todo)[i]->cost();
				if (tpc > maxTPC)
				{
					maxTPC = tpc;
					indexMaxTPC = i;
				}
			}
		}
		double c = (*todo)[indexMaxTPC]->cost();

		// if the cost of ride isnt over total_cost then add ride into result
		if ((result_cost + c) <= total_cost)
		{
			(*result).push_back((*todo)[indexMaxTPC]);
			result_cost += c;
		}
		(*todo).erase((*todo).begin() + indexMaxTPC);
	}
	return result;
}


// Compute the optimal set of ride items with an exhaustive search algorithm.
// Specifically, among all subsets of ride items, return the subset whose cost 
// in dollars fits within the total_cost budget and whose total time is greatest.
// To avoid overflow, the size of the ride items vector must be less than 64.
std::unique_ptr<RideVector> exhaustive_max_time
(
	const RideVector& rides,
	double total_cost
)
{
	std::unique_ptr<RideVector> best1(new RideVector);
	int n = rides.size();
	double candidateTotalCost = 0;
	double candidateTotalTime = 0;
	double bestTotalCost = 0;
	double bestTotalTime = 0;

	//ride items vector must be less than 64 to avoid overflow
	if (rides.size() >= 64)
	{
		exit(1);	// if ride size is greater than 64, exit program
	}

	for (uint64_t bits = 0; bits < pow(2, n); bits++)
	{
		std::unique_ptr<RideVector> candidate1(new RideVector);

		for (int j = 0; j < n; j++)
		{
			if (((bits >> j) & 1) == 1)
			{
				(*candidate1).push_back(rides[j]);
			}
		}
		
		// calculate total cost and total time of candidate and best
		sum_ride_vector(*candidate1, candidateTotalCost, candidateTotalTime);
		sum_ride_vector(*best1, bestTotalCost, bestTotalTime);

		// move candidate to best if within budget and has greater total time than current best
		if (candidateTotalCost <= total_cost)
		{
			if ((*best1).empty() || candidateTotalTime > bestTotalTime)
			{
				*best1 = *candidate1;
			}
		}
	}
// 	// do slow initialization before creating a Timer
//    Timer timer;
//    // timer is now running, immediately run the code you want timed
//    double elapsed = timer.elapsed();
//    std::cout << "Elapsed time in seconds: " << elapsed << std::endl;
	return best1;
}
