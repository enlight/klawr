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

#include "KlawrRuntimePluginPrivatePCH.h"
#include "KlawrObjectReferencer.h"

namespace Klawr {

FObjectReferencer* FObjectReferencer::Singleton = nullptr;

void FObjectReferencer::Startup()
{
	check(!Singleton);

	Singleton = new FObjectReferencer();
}

void FObjectReferencer::Shutdown()
{
	if (Singleton)
	{
		delete Singleton;
		Singleton = nullptr;
	}
}

void FObjectReferencer::AddObjectRef(UObject* obj)
{
	check(Singleton);

	auto annotation = Singleton->ObjectRefs.GetAnnotation(obj);
	++annotation.Count;
	Singleton->ObjectRefs.AddAnnotation(obj, annotation);
}

void FObjectReferencer::RemoveObjectRef(UObject* obj)
{
	check(Singleton);

	auto annotation = Singleton->ObjectRefs.GetAnnotation(obj);
	if (annotation.Count > 0)
	{
		--annotation.Count;
	}
	Singleton->ObjectRefs.AddAnnotation(obj, annotation);
}

void FObjectReferencer::AddReferencedObjects(FReferenceCollector& Collector)
{
	// don't want the collector to NULL pointers to UObject(s) marked for destruction
	Collector.AllowEliminatingReferences(false);
	for (auto& Iterator : ObjectRefs.GetAnnotationMap())
	{
		// annotations with a ref count of zero shouldn't be in the map
		check(Iterator.Value.Count > 0);
		UObjectBase* obj = const_cast<UObjectBase*>(Iterator.Key);
		Collector.AddReferencedObject(obj);
	}
	Collector.AllowEliminatingReferences(true);
}

} // namespace KlawrObjectUtils
