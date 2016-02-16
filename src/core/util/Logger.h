/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
//$Id$

/*
 * The Software is made available solely for use according to the License Agreement. Any reproduction
 * or redistribution of the Software not in accordance with the License Agreement is expressly prohibited
 * by law, and may result in severe civil and criminal penalties. Violators will be prosecuted to the
 * maximum extent possible.
 *
 * THE SOFTWARE IS WARRANTED, IF AT ALL, ONLY ACCORDING TO THE TERMS OF THE LICENSE AGREEMENT. EXCEPT
 * AS WARRANTED IN THE LICENSE AGREEMENT, SRCH2 INC. HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS WITH
 * REGARD TO THE SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES AND CONDITIONS OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT.  IN NO EVENT SHALL SRCH2 INC. BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
 * OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF SOFTWARE.

 * Copyright © 2010 SRCH2 Inc. All rights reserved
 */

#ifndef __FUNC_LINE__

#ifndef NDEBUG
#define __FUNC_LINE__
/* #define __FUNC_LINE__ \
//	{\
//	string fileName(__FILE__);\
//	fileName = fileName.substr(fileName.find_last_of('/'));\
//	stringstream callInfo;\
//	callInfo << fileName << "/" << __func__ << "/" << __LINE__ ;\
//	const char * __func_line__ = callInfo.str().c_str();\
//	Logger::sharding(Logger::FuncLine, "%s", __func_line__);\\
//	}\ */

#else
#define __FUNC_LINE__
#endif

#endif

#ifndef __SRCH2_UTIL_LOG_H__
#define __SRCH2_UTIL_LOG_H__

#include <cstdio>

namespace srch2 {
namespace util {

class Logger {
public:
	typedef enum LogLevel {
		SRCH2_LOG_SILENT = 0,
		SRCH2_LOG_ERROR = 1,
		SRCH2_LOG_WARN = 2,
		SRCH2_LOG_INFO = 3,
		SRCH2_LOG_DEBUG = 4
	} LogLevel;

	static const int kMaxLengthOfMessage = 1024*4;
private:
	static LogLevel _logLevel;
	static FILE* _out_file;

	static char* formatCurrentTime(char* buffer, unsigned size);
	static char* formatLogString(char* buffer, const char* prefix);
	static void writeToFile(FILE* out, const char* str);
public:
        // Logger now owns FILE * - it may close it (for example, upon /resetLogger request)
	static inline void setOutputFile(FILE* file) {
		if (file != NULL){
			_out_file = file;
		}else{
			Logger::warn("The given logger file is NULL");
		}
	}
	static inline void setLogLevel(LogLevel logLevel) {
		_logLevel = logLevel;
	}
	static inline int getLogLevel() {
		return _logLevel;
	}

	enum ShardingLogLevel{
		Error,
		Warning,
		Step,
		Detail,
		Info,
		FuncLine
	};
	static void console(const char *format, ...);
	static void error(const char *format, ...);
	static void warn(const char *format, ...);
	static void info(const char *format, ...);
	static void debug(const char *format, ...);
	static void sharding(ShardingLogLevel logLevel, const char *format, ...);
	static FILE* swapLoggerFile(FILE * newLogger);

        static void close() { if (_out_file != NULL) { fclose(_out_file);} }
};

}
}

#endif /* _SRCH2_UTIL_LOG_H_ */
