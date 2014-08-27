//-------------------------------------------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2014 Vadim Macagon
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//-------------------------------------------------------------------------------
#pragma once

namespace Klawr {

/** 
 * @brief Keeps alive native UObject instances referenced by managed code.
 * 
 * FObjectReferencer lets the garbage collector know which native UObject instances are currently
 * referenced by managed code so that they aren't garbage collected. Multiple managed objects may
 * reference a single native UObject so a reference count is maintained for each UObject that 
 * crosses the native/managed code boundary.
 */
class FObjectReferencer : public FGCObject
{
public:
	static void Startup();
	static void Shutdown();

	static void AddObjectRef(UObject* obj);
	static void RemoveObjectRef(UObject* obj);

	
public: // FGCObject interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

private:
	/** An annotation containing a reference count. */
	struct FRefCountAnnotation
	{
		/** Current number of references to a native UObject instance in managed code. */
		uint32 Count;

		/** Construct an annotation with the default value. */
		FRefCountAnnotation()
			: Count(0)
		{
		}

		/** Check if this annotation contains the default value. */
		bool IsDefault() const
		{
			return (Count == 0);
		}
	};

	FUObjectAnnotationSparse<FRefCountAnnotation, false /*bAutoRemove*/> ObjectRefs;
	static FObjectReferencer* Singleton;
};

} // namespace Klawr
