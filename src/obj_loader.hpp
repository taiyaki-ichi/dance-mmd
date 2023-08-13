#pragma once
#include<istream>
#include<array>
#include<vector>
#include<regex>
#include<string>

// position, normal
inline std::vector<std::array<float, 6>> load_obj(std::istream& in)
{
	const std::regex commentout{ "(.*)#(.*)" };
	const std::regex position{ "v\\s([+-]?\\d+(?:\\.\\d+)?(?:[eE][+-]?\\d+)?)\\s([+-]?\\d+(?:\\.\\d+)?(?:[eE][+-]?\\d+)?)\\s([+-]?\\d+(?:\\.\\d+)?(?:[eE][+-]?\\d+)?)" };
	const std::regex normal{ "vn\\s([+-]?\\d+(?:\\.\\d+)?(?:[eE][+-]?\\d+)?)\\s([+-]?\\d+(?:\\.\\d+)?(?:[eE][+-]?\\d+)?)\\s([+-]?\\d+(?:\\.\\d+)?(?:[eE][+-]?\\d+)?)" };
	const std::regex face{ "f\\s(\\d+)//(\\d+)\\s(\\d+)//(\\d+)\\s(\\d+)//(\\d+)" };
	const std::regex face2{ "f\\s(\\d+)/(\\d+)/(\\d+)\\s(\\d+)/(\\d+)/(\\d+)\\s(\\d+)/(\\d+)/(\\d+)" };

	std::vector<std::array<float, 3>> position_data{};
	std::vector<std::array<float, 3>> normal_data{};

	std::vector<std::array<float, 6>> result{};

	std::string buffer{};
	std::smatch match{};

	while (std::getline(in, buffer))
	{
		// コメントアウト
		if (std::regex_match(buffer, match, commentout))
		{
			buffer = match[1].str();
		}

		if (std::regex_match(buffer, match, position))
		{
			position_data.push_back({ std::stof(match[1].str()), std::stof(match[2].str()), std::stof(match[3].str()) });
		}
		else if (std::regex_match(buffer, match, normal))
		{
			normal_data.push_back({ std::stof(match[1].str()), std::stof(match[2].str()), std::stof(match[3].str()) });
		}
		else if (std::regex_match(buffer, match, face))
		{
			auto const& position1 = position_data[std::stoi(match[1].str()) - 1];
			auto const& normal1 = normal_data[std::stoi(match[2].str()) - 1];
			auto const& position2 = position_data[std::stoi(match[3].str()) - 1];
			auto const& normal2 = normal_data[std::stoi(match[4].str()) - 1];
			auto const& position3 = position_data[std::stoi(match[5].str()) - 1];
			auto const& normal3 = normal_data[std::stoi(match[6].str()) - 1];

			result.push_back({
				position1[0], position1[1], position1[2],
				normal1[0], normal1[1], normal1[2],
				});

			result.push_back({
				position2[0], position2[1], position2[2],
				normal2[0], normal2[1], normal2[2],
				});

			result.push_back({
				position3[0], position3[1], position3[2],
				normal3[0], normal3[1], normal3[2],
				});
		}
		else if (std::regex_match(buffer, match, face2))
		{
			auto const& position1 = position_data[std::stoi(match[1].str()) - 1];
			auto const& normal1 = normal_data[std::stoi(match[3].str()) - 1];
			auto const& position2 = position_data[std::stoi(match[4].str()) - 1];
			auto const& normal2 = normal_data[std::stoi(match[6].str()) - 1];
			auto const& position3 = position_data[std::stoi(match[7].str()) - 1];
			auto const& normal3 = normal_data[std::stoi(match[9].str()) - 1];

			result.push_back({
				position1[0], position1[1], position1[2],
				normal1[0], normal1[1], normal1[2],
				});

			result.push_back({
				position2[0], position2[1], position2[2],
				normal2[0], normal2[1], normal2[2],
				});

			result.push_back({
				position3[0], position3[1], position3[2],
				normal3[0], normal3[1], normal3[2],
				});
		}
	}

	return result;
}