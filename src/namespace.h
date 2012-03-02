/** @file
* @brief namespace definition
* @author yafei.zhang@langtaojin.com
* @date
* @version
*
*/
#ifndef _LANGTAOJIN_ADGAGA_NAMESPACE_H_
#define _LANGTAOJIN_ADGAGA_NAMESPACE_H_

#if defined DISABLE_NAMESPACE
# define NAMESPACE_BEGIN
# define NAMESPACE_END
# define USING_NAMESPACE
//#elif defined NAMESPACE
//# define NAMESPACE_BEGIN namespace libredis {
//# define NAMESPACE_END }
//# define USING_NAMESPACE using namespace libredis;
#else
# define NAMESPACE_BEGIN namespace com { namespace langtaojin { namespace adgaga {
# define NAMESPACE_END } } }
# define USING_NAMESPACE using namespace com::langtaojin::adgaga;
#endif

#endif //_LANGTAOJIN_ADGAGA_NAMESPACE_H_
