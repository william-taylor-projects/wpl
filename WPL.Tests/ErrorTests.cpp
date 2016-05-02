
#include "CppUnitTest.h"
#include "Tests.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WPLTests
{		
	TEST_CLASS(ErrorTests)
	{
	public:
		TEST_METHOD(TestCannotFindFile)
		{
			wpl::VideoPlayer videoPlayer;

			if(videoPlayer.openVideo("doesntexists.wmv"))
			{
				Assert::Fail(L"Error this file doesnt exist and should return false to indicate failure");
			}
		}
	};
}