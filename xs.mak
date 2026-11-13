# Moddable XS (JavaScript engine) for Open Watcom - object list and build rules
# Optimization flags come from MBOPT in the main makefile. Platform: xs\w95 (w95pre.h is force-included).

XSFLAGS = -q -bt=nt $(MBOPT) -bm -w0 -za99 -fi=xs\w95\w95pre.h -ixs\w95 -ixs\includes -ixs\platforms -ixs\sources -ixs\fdlibm

XS_OBJS = &
    xobj\xsAll.obj &
    xobj\xsAPI.obj &
    xobj\xsArguments.obj &
    xobj\xsArray.obj &
    xobj\xsAtomics.obj &
    xobj\xsBigInt.obj &
    xobj\xsBoolean.obj &
    xobj\xsCode.obj &
    xobj\xsCommon.obj &
    xobj\xsDataView.obj &
    xobj\xsDate.obj &
    xobj\xsDebug.obj &
    xobj\xsDefaults.obj &
    xobj\xsError.obj &
    xobj\xsFunction.obj &
    xobj\xsGenerator.obj &
    xobj\xsGlobal.obj &
    xobj\xsJSON.obj &
    xobj\xsLexical.obj &
    xobj\xsLockdown.obj &
    xobj\xsMapSet.obj &
    xobj\xsMarshall.obj &
    xobj\xsMath.obj &
    xobj\xsMemory.obj &
    xobj\xsModule.obj &
    xobj\xsNumber.obj &
    xobj\xsObject.obj &
    xobj\xsPlatforms.obj &
    xobj\xsProfile.obj &
    xobj\xsPromise.obj &
    xobj\xsProperty.obj &
    xobj\xsProxy.obj &
    xobj\xsRegExp.obj &
    xobj\xsRun.obj &
    xobj\xsScope.obj &
    xobj\xsScript.obj &
    xobj\xsSourceMap.obj &
    xobj\xsString.obj &
    xobj\xsSymbol.obj &
    xobj\xsSyntaxical.obj &
    xobj\xsTree.obj &
    xobj\xsType.obj &
    xobj\xsdtoa.obj &
    xobj\xsre.obj &
    xobj\e_acos.obj &
    xobj\e_acosh.obj &
    xobj\e_asin.obj &
    xobj\e_atan2.obj &
    xobj\e_atanh.obj &
    xobj\e_cosh.obj &
    xobj\e_exp.obj &
    xobj\e_fmod.obj &
    xobj\e_hypot.obj &
    xobj\e_log.obj &
    xobj\e_log10.obj &
    xobj\e_pow.obj &
    xobj\e_rem_pio2.obj &
    xobj\e_sinh.obj &
    xobj\k_cos.obj &
    xobj\k_exp.obj &
    xobj\k_rem_pio2.obj &
    xobj\k_sin.obj &
    xobj\k_tan.obj &
    xobj\s_asinh.obj &
    xobj\s_atan.obj &
    xobj\s_cbrt.obj &
    xobj\s_ceil.obj &
    xobj\s_cos.obj &
    xobj\s_expm1.obj &
    xobj\s_log1p.obj &
    xobj\s_scalbn.obj &
    xobj\s_sin.obj &
    xobj\s_tan.obj &
    xobj\s_tanh.obj &
    xobj\s_trunc.obj &
    xobj\w95_xs.obj

xs.lib: $(XS_OBJS)
	wlib -q -n -b xs.lib @xs.lbc

xobj\xsAll.obj: xs\sources\xsAll.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsAll.c

xobj\xsAPI.obj: xs\sources\xsAPI.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsAPI.c

xobj\xsArguments.obj: xs\sources\xsArguments.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsArguments.c

xobj\xsArray.obj: xs\sources\xsArray.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsArray.c

xobj\xsAtomics.obj: xs\sources\xsAtomics.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsAtomics.c

xobj\xsBigInt.obj: xs\sources\xsBigInt.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsBigInt.c

xobj\xsBoolean.obj: xs\sources\xsBoolean.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsBoolean.c

xobj\xsCode.obj: xs\sources\xsCode.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsCode.c

xobj\xsCommon.obj: xs\sources\xsCommon.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsCommon.c

xobj\xsDataView.obj: xs\sources\xsDataView.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsDataView.c

xobj\xsDate.obj: xs\sources\xsDate.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsDate.c

xobj\xsDebug.obj: xs\sources\xsDebug.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsDebug.c

xobj\xsDefaults.obj: xs\sources\xsDefaults.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsDefaults.c

xobj\xsError.obj: xs\sources\xsError.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsError.c

xobj\xsFunction.obj: xs\sources\xsFunction.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsFunction.c

xobj\xsGenerator.obj: xs\sources\xsGenerator.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsGenerator.c

xobj\xsGlobal.obj: xs\sources\xsGlobal.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsGlobal.c

xobj\xsJSON.obj: xs\sources\xsJSON.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsJSON.c

xobj\xsLexical.obj: xs\sources\xsLexical.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsLexical.c

xobj\xsLockdown.obj: xs\sources\xsLockdown.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsLockdown.c

xobj\xsMapSet.obj: xs\sources\xsMapSet.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsMapSet.c

xobj\xsMarshall.obj: xs\sources\xsMarshall.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsMarshall.c

xobj\xsMath.obj: xs\sources\xsMath.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsMath.c

xobj\xsMemory.obj: xs\sources\xsMemory.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsMemory.c

xobj\xsModule.obj: xs\sources\xsModule.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsModule.c

xobj\xsNumber.obj: xs\sources\xsNumber.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsNumber.c

xobj\xsObject.obj: xs\sources\xsObject.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsObject.c

xobj\xsPlatforms.obj: xs\sources\xsPlatforms.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsPlatforms.c

xobj\xsProfile.obj: xs\sources\xsProfile.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsProfile.c

xobj\xsPromise.obj: xs\sources\xsPromise.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsPromise.c

xobj\xsProperty.obj: xs\sources\xsProperty.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsProperty.c

xobj\xsProxy.obj: xs\sources\xsProxy.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsProxy.c

xobj\xsRegExp.obj: xs\sources\xsRegExp.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsRegExp.c

xobj\xsRun.obj: xs\sources\xsRun.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsRun.c

xobj\xsScope.obj: xs\sources\xsScope.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsScope.c

xobj\xsScript.obj: xs\sources\xsScript.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsScript.c

xobj\xsSourceMap.obj: xs\sources\xsSourceMap.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsSourceMap.c

xobj\xsString.obj: xs\sources\xsString.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsString.c

xobj\xsSymbol.obj: xs\sources\xsSymbol.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsSymbol.c

xobj\xsSyntaxical.obj: xs\sources\xsSyntaxical.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsSyntaxical.c

xobj\xsTree.obj: xs\sources\xsTree.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsTree.c

xobj\xsType.obj: xs\sources\xsType.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsType.c

xobj\xsdtoa.obj: xs\sources\xsdtoa.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsdtoa.c

xobj\xsre.obj: xs\sources\xsre.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\sources\xsre.c

xobj\e_acos.obj: xs\fdlibm\e_acos.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\fdlibm\e_acos.c

xobj\e_acosh.obj: xs\fdlibm\e_acosh.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\fdlibm\e_acosh.c

xobj\e_asin.obj: xs\fdlibm\e_asin.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\fdlibm\e_asin.c

xobj\e_atan2.obj: xs\fdlibm\e_atan2.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\fdlibm\e_atan2.c

xobj\e_atanh.obj: xs\fdlibm\e_atanh.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\fdlibm\e_atanh.c

xobj\e_cosh.obj: xs\fdlibm\e_cosh.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\fdlibm\e_cosh.c

xobj\e_exp.obj: xs\fdlibm\e_exp.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\fdlibm\e_exp.c

xobj\e_fmod.obj: xs\fdlibm\e_fmod.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\fdlibm\e_fmod.c

xobj\e_hypot.obj: xs\fdlibm\e_hypot.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\fdlibm\e_hypot.c

xobj\e_log.obj: xs\fdlibm\e_log.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\fdlibm\e_log.c

xobj\e_log10.obj: xs\fdlibm\e_log10.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\fdlibm\e_log10.c

xobj\e_pow.obj: xs\fdlibm\e_pow.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\fdlibm\e_pow.c

xobj\e_rem_pio2.obj: xs\fdlibm\e_rem_pio2.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\fdlibm\e_rem_pio2.c

xobj\e_sinh.obj: xs\fdlibm\e_sinh.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\fdlibm\e_sinh.c

xobj\k_cos.obj: xs\fdlibm\k_cos.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\fdlibm\k_cos.c

xobj\k_exp.obj: xs\fdlibm\k_exp.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\fdlibm\k_exp.c

xobj\k_rem_pio2.obj: xs\fdlibm\k_rem_pio2.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\fdlibm\k_rem_pio2.c

xobj\k_sin.obj: xs\fdlibm\k_sin.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\fdlibm\k_sin.c

xobj\k_tan.obj: xs\fdlibm\k_tan.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\fdlibm\k_tan.c

xobj\s_asinh.obj: xs\fdlibm\s_asinh.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\fdlibm\s_asinh.c

xobj\s_atan.obj: xs\fdlibm\s_atan.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\fdlibm\s_atan.c

xobj\s_cbrt.obj: xs\fdlibm\s_cbrt.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\fdlibm\s_cbrt.c

xobj\s_ceil.obj: xs\fdlibm\s_ceil.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\fdlibm\s_ceil.c

xobj\s_cos.obj: xs\fdlibm\s_cos.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\fdlibm\s_cos.c

xobj\s_expm1.obj: xs\fdlibm\s_expm1.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\fdlibm\s_expm1.c

xobj\s_log1p.obj: xs\fdlibm\s_log1p.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\fdlibm\s_log1p.c

xobj\s_scalbn.obj: xs\fdlibm\s_scalbn.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\fdlibm\s_scalbn.c

xobj\s_sin.obj: xs\fdlibm\s_sin.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\fdlibm\s_sin.c

xobj\s_tan.obj: xs\fdlibm\s_tan.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\fdlibm\s_tan.c

xobj\s_tanh.obj: xs\fdlibm\s_tanh.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\fdlibm\s_tanh.c

xobj\s_trunc.obj: xs\fdlibm\s_trunc.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\fdlibm\s_trunc.c

xobj\w95_xs.obj: xs\w95\w95_xs.c xs\w95\w95_xs.h
	@if not exist xobj mkdir xobj
	wcc386 $(XSFLAGS) -fo=$@ xs\w95\w95_xs.c

