import pydmtools as pydm
import tempfile
import os
import sys
import hashlib
import numpy as np

print(pydm.numpy)
class TestRemote():
    #fname = "http://raw.githubusercontent.com/dpryan79/pydm/master/pydmTest/test.dm"
    fname=""

    def doOpen(self):
        dm = pydm.openfile(self.fname)
        assert(dm is not None)
        return dm

    def doOpenWith(self):
        with pydm.openfile(self.fname) as dm:
            print(dm.chroms())
            #assert(dm.chroms() == {'1': 195471971, '10': 130694993})

    def doChroms(self, dm):
        #assert(dm.chroms() == {'1': 195471971, '10': 130694993})
        #assert(dm.chroms("1") == 195471971)
        #assert(dm.chroms("c") is None)
        print(dm.chroms())
        print("dm.chroms")
        print(dm.chroms("chr1"))
        print("cc")
        print(dm.chroms("c"))

    def doHeader(self, dm):
        #assert(dm.header() == {'maxVal': 2, 'sumData': 272, 'minVal': 0, 'version': 4, 'sumSquared': 500, 'nLevels': 1, 'nBasesCovered': 154})
        print(dm.header())

    def doStats(self, dm):
        #type mean median max min std dev coverage cov sum
        #assert(dm.stats("1", 0, 3) == [0.2000000054637591])
        #assert(dm.stats("1", 0, 3, type="max") == [0.30000001192092896])
        #assert(dm.stats("1",99,200, type="max", nBins=2) == [1.399999976158142, 1.5])
        #assert(dm.stats("1",np.int64(99), np.int64(200), type="max", nBins=2) == [1.399999976158142, 1.5])
        #assert(dm.stats("1") == [1.3351851569281683])
        ##print(dm.stats("chr1"))
        print('dm.stats')
        #dm.stats("chr1")
        print(dm.stats("chr1", 94942, 100000))

    def doValues(self, dm):
        print("value")
        print(dm.getvalues("chr1", 9, 100000))
        #assert(dm.getvalues("1", 0, 3) == [0.10000000149011612, 0.20000000298023224, 0.30000001192092896])
        #assert(dm.getvalues("1", np.int64(0), np.int64(3)) == [0.10000000149011612, 0.20000000298023224, 0.30000001192092896])
        #assert(dm.getvalues("1", 0, 4) == [0.10000000149011612, 0.20000000298023224, 0.30000001192092896, 'nan'])

    def doIntervals(self, dm):
        print("intv")
        print(dm.intervals("chr1", 94942,94990))
        #assert(dm.intervals("1", 0, 3) == ((0, 1, 0.10000000149011612), (1, 2, 0.20000000298023224), (2, 3, 0.30000001192092896)))
        #assert(dm.intervals("1", np.int64(0), np.int64(3)) == ((0, 1, 0.10000000149011612), (1, 2, 0.20000000298023224), (2, 3, 0.30000001192092896)))
        #assert(dm.intervals("1") == ((0, 1, 0.10000000149011612), (1, 2, 0.20000000298023224), (2, 3, 0.30000001192092896), (100, 150, 1.399999976158142), (150, 151, 1.5)))

    def doSum(self, dm):
        print('sum')
        print(dm.stats("chr1", 100, 151000, type="mean", nBins=1))
        #print(dm.stats("chr1", 100, 151000, type="weight", nBins=1))

    def doWrite(self, dm):
        print("zzzz\n")
        #ofile = tempfile.NamedTemporaryFile(delete=False)
        oname = "te.dm" #ofile.name
        #ofile.close()
        dm2 = pydm.openfile(oname, "w", end="Y")

        assert(dm2 is not None)
        print("ccc\n")
        print(dm.chroms("chr1"))
        print("aaa\n")
        #Since this is an unordered dict(), iterating over the items can swap the order!
        chroms = [("chr1", dm.chroms("chr1")), ("chr10", dm.chroms("chr10"))]
        print(len(dm.chroms()))
        dm2.addHeader(chroms, maxZooms=1)
        #Copy the input file
        for c in chroms:
            ints = dm.intervals(c[0])
            chroms2 = []
            starts = []
            ends = []
            values = []
            if ints:
                for entry in ints:
                    chroms2.append(c[0])
                    starts.append(entry[0])
                    ends.append(entry[1])
                    values.append(entry[2])
                #print("aa write", chroms2, starts, ends, values)
                dm2.addEntries(chroms2, starts, ends=ends, values=values)
        dm2.close()
        print("closed")
        #Ensure that the copied file has the same entries and max/min/etc.
        dm2 = pydm.openfile(oname)
        print(dm.header())
        print(dm2.header())
        print(dm.chroms())
        print(dm2.chroms())
        for c in chroms:
            ints1 = dm.intervals(c[0])
            ints2 = dm2.intervals(c[0])
            assert(ints1 == ints2)
        dm.close()
        dm2.close()
        #Clean up
        os.remove(oname)

    def doWrite2(self):
        '''
        Test all three modes of storing entries. Also test to ensure that we get error messages when doing something silly

        This is a modified version of the writing example from libBigWig
        '''
        chroms = ["chr1"]*6
        starts = [0, 100, 125, 200, 220, 230, 500, 600, 625, 700, 800, 850]
        ends = [5, 120, 126, 205, 226, 231]
        values = [0.0, 1.0, 200.0, -2.0, 150.0, 25.0, 0.0, 1.0, 200.0, -2.0, 150.0, 25.0, -5.0, -20.0, 25.0, -5.0, -20.0, 25.0]
        ofile = tempfile.NamedTemporaryFile(delete=False)
        oname = ofile.name
        ofile.close()
        dm = pydm.openfile(oname, "w", end="Y")
        print("oname", oname)
        dm.addHeader([("chr1", 1000000), ("chr2", 1500000)])
        print("add hdr correct")

        #Intervals
        dm.addEntries(chroms[0:3], starts[0:3], ends=ends[0:3], values=values[0:3])
        dm.addEntries(chroms[3:6], starts[3:6], ends=ends[3:6], values=values[3:6])

        print("add 1 suc")
        #IntervalSpans, not valid in dm
        #dm.addEntries("chr1", starts[6:9], values=values[6:9], span=20)
        #dm.addEntries("chr1", starts[9:12], values=values[9:12], span=20)

        #IntervalSpanSteps, this should instead take an int
        #dm.addEntries("chr1", 900, values=values[12:15], span=20, step=30)
        #dm.addEntries("chr1", 990, values=values[15:18], span=20, step=30)

        print("add suscs")

        #Attempt to add incorrect values. These MUST raise an exception
        try:
            dm.addEntries(chroms[0:3], starts[0:3], ends=ends[0:3], values=values[0:3])
            assert(1==0)
        except RuntimeError:
            pass
        #try:
        #    dm.addEntries("chr1", starts[6:9], values=values[6:9], span=20)
        #    assert(1==0)
        #except RuntimeError:
        #    pass
        #try:
        #    dm.addEntries("chrxx", starts[6:9], values=values[6:9], span=20)
        #    assert(1==0)
        #except RuntimeError:
        #    pass
        #try:
        #    dm.addEntries("chr1", 900, values=values[12:15], span=20, step=30)
        #    assert(1==0)
        #except RuntimeError:
        #    pass

        #Add a few intervals on a new chromosome
        dm.addEntries(["chr2"]*3, starts[0:3], ends=ends[0:3], values=values[0:3])
        dm.close()
        #check md5sum, this is the simplest method to check correctness
        #h = hashlib.md5(open(oname, "rb").read()).hexdigest()
        #assert(h=="ef104f198c6ce8310acc149d0377fc16")
        #Clean up
        os.remove(oname)

    def doWriteEmpty(self):
        ofile = tempfile.NamedTemporaryFile(delete=False)
        oname = ofile.name
        ofile.close()
        dm = pydm.openfile(oname, "w", end="Y")
        dm.addHeader([("1", 1000000), ("2", 1500000)])
        dm.close()

        #check md5sum
        h = hashlib.md5(open(oname, "rb").read()).hexdigest()
        print(h)
        #assert(h=="361c600e5badf0b45d819552a7822937")

        #Ensure we can openfile and get reasonable results
        dm = pydm.openfile(oname)
        assert(dm.chroms() == {'1': 1000000, '2': 1500000})
        assert(dm.intervals("1") == None)
        assert(dm.getvalues("1", 0, 1000000) == [])
        assert(dm.stats("1", 0, 1000000, nBins=2) == [None, None])
        dm.close()

        #Clean up
        os.remove(oname)

    def doWriteNumpy(self):
        ofile = tempfile.NamedTemporaryFile(delete=False)
        oname = ofile.name
        ofile.close()
        dm = pydm.openfile(oname, "w", end="Y")
        dm.addHeader([("chr1", 100), ("chr2", 150), ("chr3", 200), ("chr4", 250)])
        chroms = np.array(["chr1"] * 2 + ["chr2"] * 2 + ["chr3"] * 2 + ["chr4"] * 2)
        starts = np.array([0, 10, 40, 50, 60, 70, 80, 90], dtype=np.int64)
        ends = np.array([5, 15, 45, 55, 65, 75, 85, 95], dtype=np.int64)
        values0 = np.array(np.random.random_sample(8), dtype=np.float64)
        chroms = ['chr1', 'chr1', 'chr2', 'chr2', 'chr3', 'chr3', 'chr4', 'chr4']
        #starts = [0, 10, 40, 50, 60, 70, 80, 90]
        #ends = [5, 15, 45, 55, 65, 75, 85, 95]
        #values0 = [0.9655627009501314, 0.6156690607412797, 0.5283073859823024, 0.6515650516673249, 0.332654161864981, 0.528812268231359, 0.774339339328257, 0.6141506720391188]
        print(list(chroms), list(starts), list(ends), list(values0), oname)
        dm.addEntries(chroms, starts, ends=ends, values=values0)
        dm.close()

        vals = [(x, y, z) for x, y, z in zip(starts, ends, values0)]
        dm = pydm.openfile(oname)
        assert(dm.chroms() == {'chr1': 100, 'chr2': 150, 'chr3': 200, 'chr4': 250})
        for idx1, chrom in enumerate(["chr1", "chr2", "chr3", "chr4"]):
            for idx2, tup in enumerate(dm.intervals(chrom)):
                assert(tup[0] == starts[2 * idx1 + idx2])
                assert(tup[1] == ends[2 * idx1 + idx2])
                assert(np.isclose(tup[2], values0[2 * idx1 + idx2]))
        dm.close()

        #Clean up
        os.remove(oname)

    def testAll(self):
        print("test all")
        dm = self.doOpen()
        self.doChroms(dm)
        if not self.fname.startswith("http"):
            self.doHeader(dm)
            self.doStats(dm)
            self.doSum(dm)
            self.doValues(dm)
            self.doIntervals(dm)
            self.doWrite(dm)
            self.doOpenWith()
            print("self.doOpenWith")
            self.doWrite2()
            print("self.doWrite2")
            self.doWriteEmpty()
            print("doWriteEmpty ee")
            self.doWriteNumpy()
        dm.close()

class TestLocal():
    def testFoo(self):
        blah = TestRemote()
        blah.fname = os.path.dirname(pydm.__file__) + "/pydmtest/test.dm"
        blah.testAll()

print("xxxx")
#tl=TestLocal()
#tl.testFoo()

class TestNumpy():
    def testNumpy(self):
        import os
        if pydm.numpy == 0:
            return 0
        import numpy as np

        dm = pydm.openfile("pydmtest/testnp.dm", "w")
        dm.addHeader([("chr1", 1000)], maxZooms=0)
        # Type 0
        chroms = np.array(["chr1"] * 10)
        starts = np.array([0, 10, 20, 30, 40, 50, 60, 70, 80, 90], dtype=np.int64)
        ends = np.array([5, 15, 25, 35, 45, 55, 65, 75, 85, 95], dtype=np.int64)
        values0 = np.array(np.random.random_sample(10), dtype=np.float64)
        print(chroms, starts, ends, values0, "testNumpy")
        dm.addEntries(chroms, starts, ends=ends, values=values0)

        starts = np.array([100, 110, 120, 130, 140, 150, 160, 170, 180, 190], dtype=np.int64)
        ends = np.array([105, 115, 125, 135, 145, 155, 165, 175, 185, 195], dtype=np.int64)
        values1 = np.array(np.random.random_sample(10), dtype=np.float64)
        dm.addEntries(chroms, starts, ends=ends, values=values1)

        # Type 1, single chrom, multiple starts/values, single span
        #starts = np.array([200, 210, 220, 230, 240, 250, 260, 270, 280, 290], dtype=np.int64)
        #values2 = np.array(np.random.random_sample(10), dtype=np.float64)
        #dm.addEntries(np.str("chr1"), starts, span=np.int(8), values=values2)

        #starts = np.array([300, 310, 320, 330, 340, 350, 360, 370, 380, 390], dtype=np.int64)
        #values3 = np.array(np.random.random_sample(10), dtype=np.float64)
        #dm.addEntries(np.str("chr1"), starts, span=np.int(8), values=values3)

        # Type 2, single chrom/start/span/step, multiple values
        #values4 = np.array(np.random.random_sample(10), dtype=np.float64)
        #dm.addEntries(np.str("chr1"), np.int(400), span=np.int(8), step=np.int64(2), values=values4)

        #values5 = np.array(np.random.random_sample(10), dtype=np.float64)
        #dm.addEntries(np.str("chr1"), np.int(500), span=np.int(8), step=np.int64(2), values=values5)

        dm.close()

        dm = pydm.openfile("pydmtest/testnp.dm")
        assert(dm is not None)

        def compy(start, v2):
            v = []
            for t in dm.intervals("chr1", start, start + 100):
                v.append(t[2])
            v = np.array(v)
            assert(np.all(np.abs(v - v2) < 1e-5))

        compy(0, values0)
        compy(100, values1)
        #compy(200, values2)
        #compy(300, values3)
        #compy(400, values4)
        #compy(500, values5)

        # Get values as a numpy array
        foo = dm.getvalues("chr1", 0, 100, numpy=False)
        print(foo, "foo")
        assert(isinstance(foo, list))
        foo = dm.getvalues("chr1", 0, 100, numpy=True)
        print(foo, "foo T")
        assert(isinstance(foo, np.ndarray))

        dm.close()
        #os.remove("test/outnp.dm")

    def testNumpyValues(self):
        if pydm.numpy == 0:
            return 0
        import numpy as np

        fname = "pydmtest/testnp.dm"
        dm = pydm.openfile(fname, "r")

        print(dm.getvalues("chr1", 0, 100, numpy=True))
        assert np.allclose(
            dm.getvalues("chr1", 0, 100, numpy=True),
            np.array(dm.getvalues("chr1", 0, 100), dtype=np.float32),
            equal_nan=True
        )

        assert np.allclose(
            dm.stats("chr1", 0, 20, "mean", 5, numpy=True),
            np.array(dm.stats("chr1", 0, 20, "mean", 5), dtype=np.float64),
            equal_nan=True
        )

tn=TestNumpy()
print("pynumpy", pydm.numpy)
tn.testNumpy()
tn.testNumpyValues()
