/*  Copyright 2017 - 2018 Amazon.com, Inc. or its affiliates.All Rights Reserved.
Licensed under the Amazon Software License(the "License").You may not use
this file except in compliance with the License.A copy of the License is
located at

http://aws.amazon.com/asl/

and in the "LICENSE" file accompanying this file.This file is distributed
on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, express
or implied.See the License for the specific language governing
permissions and limitations under the License. */

#pragma once
#include <vector>

class PollySpeechResponse
{
public:
	std::streamsize Length;
	std::vector<unsigned char> AudioData = std::vector<unsigned char>(5000000);
	std::string ErrorMessage ;
	bool IsSuccess;
};
