import pybmtools as pybm
import tempfile
import os
import sys
import hashlib
import numpy as np

class TestRemote():
    #fname = "http://raw.githubusercontent.com/dpryan79/pybm/master/pybmTest/test.bw"
    fname=""

    def doOpen(self):
        bw = pybm.openfile(self.fname)
        assert(bw is not None)
        return bw

    def doOpenWith(self):
        with pybm.openfile(self.fname) as bw:
            print(bw.chroms())
            #assert(bw.chroms() == {'1': 195471971, '10': 130694993})

    def doChroms(self, bw):
        #assert(bw.chroms() == {'1': 195471971, '10': 130694993})
        #assert(bw.chroms("1") == 195471971)
        #assert(bw.chroms("c") is None)
        print(bw.chroms())
        print("bw.chroms")
        print(bw.chroms("chr1"))
        print("cc")
        print(bw.chroms("c"))

    def doHeader(self, bw):
        #assert(bw.header() == {'maxVal': 2, 'sumData': 272, 'minVal': 0, 'version': 4, 'sumSquared': 500, 'nLevels': 1, 'nBasesCovered': 154})
        print(bw.header())

    def doStats(self, bw):
        #type mean median max min std dev coverage cov sum
        #assert(bw.stats("1", 0, 3) == [0.2000000054637591])
        #assert(bw.stats("1", 0, 3, type="max") == [0.30000001192092896])
        #assert(bw.stats("1",99,200, type="max", nBins=2) == [1.399999976158142, 1.5])
        #assert(bw.stats("1",np.int64(99), np.int64(200), type="max", nBins=2) == [1.399999976158142, 1.5])
        #assert(bw.stats("1") == [1.3351851569281683])
        ##print(bw.stats("chr1"))
        print('bw.stats')
        #bw.stats("chr1")
        print(bw.stats("chr1", 94942, 100000))

    def doValues(self, bw):
        print("value")
        print(bw.getvalues("chr1", 9, 100000))
        #assert(bw.getvalues("1", 0, 3) == [0.10000000149011612, 0.20000000298023224, 0.30000001192092896])
        #assert(bw.getvalues("1", np.int64(0), np.int64(3)) == [0.10000000149011612, 0.20000000298023224, 0.30000001192092896])
        #assert(bw.getvalues("1", 0, 4) == [0.10000000149011612, 0.20000000298023224, 0.30000001192092896, 'nan'])

    def doIntervals(self, bw):
        print("intv")
        print(bw.intervals("chr1", 94942,94990))
        #assert(bw.intervals("1", 0, 3) == ((0, 1, 0.10000000149011612), (1, 2, 0.20000000298023224), (2, 3, 0.30000001192092896)))
        #assert(bw.intervals("1", np.int64(0), np.int64(3)) == ((0, 1, 0.10000000149011612), (1, 2, 0.20000000298023224), (2, 3, 0.30000001192092896)))
        #assert(bw.intervals("1") == ((0, 1, 0.10000000149011612), (1, 2, 0.20000000298023224), (2, 3, 0.30000001192092896), (100, 150, 1.399999976158142), (150, 151, 1.5)))

    def doSum(self, bw):
        print('sum')
        print(bw.stats("chr1", 100, 151000, type="mean", nBins=1))
        #print(bw.stats("chr1", 100, 151000, type="weight", nBins=1))

    def doWrite(self, bw):
        print("zzzz\n")
        #ofile = tempfile.NamedTemporaryFile(delete=False)
        oname = "te.bm" #ofile.name
        #ofile.close()
        bw2 = pybm.openfile(oname, "w", end="Y")

        assert(bw2 is not None)
        print("ccc\n")
        print(bw.chroms("chr1"))
        print("aaa\n")
        #Since this is an unordered dict(), iterating over the items can swap the order!
        chroms = [("chr1", bw.chroms("chr1")), ("chr10", bw.chroms("chr10"))]
        print(len(bw.chroms()))
        bw2.addHeader(chroms, maxZooms=1)
        #Copy the input file
        for c in chroms:
            ints = bw.intervals(c[0])
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
                bw2.addEntries(chroms2, starts, ends=ends, values=values)
        bw2.close()
        print("closed")
        #Ensure that the copied file has the same entries and max/min/etc.
        bw2 = pybm.openfile(oname)
        print(bw.header())
        print(bw2.header())
        print(bw.chroms())
        print(bw2.chroms())
        for c in chroms:
            ints1 = bw.intervals(c[0])
            ints2 = bw2.intervals(c[0])
            assert(ints1 == ints2)
        bw.close()
        bw2.close()
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
        bw = pybm.openfile(oname, "w", end="Y")
        print("oname", oname)
        bw.addHeader([("chr1", 1000000), ("chr2", 1500000)])
        print("add hdr correct")

        #Intervals
        bw.addEntries(chroms[0:3], starts[0:3], ends=ends[0:3], values=values[0:3])
        bw.addEntries(chroms[3:6], starts[3:6], ends=ends[3:6], values=values[3:6])

        print("add 1 suc")
        #IntervalSpans, not valid in bm
        #bw.addEntries("chr1", starts[6:9], values=values[6:9], span=20)
        #bw.addEntries("chr1", starts[9:12], values=values[9:12], span=20)

        #IntervalSpanSteps, this should instead take an int
        #bw.addEntries("chr1", 900, values=values[12:15], span=20, step=30)
        #bw.addEntries("chr1", 990, values=values[15:18], span=20, step=30)

        print("add suscs")

        #Attempt to add incorrect values. These MUST raise an exception
        try:
            bw.addEntries(chroms[0:3], starts[0:3], ends=ends[0:3], values=values[0:3])
            assert(1==0)
        except RuntimeError:
            pass
        #try:
        #    bw.addEntries("chr1", starts[6:9], values=values[6:9], span=20)
        #    assert(1==0)
        #except RuntimeError:
        #    pass
        #try:
        #    bw.addEntries("chrxx", starts[6:9], values=values[6:9], span=20)
        #    assert(1==0)
        #except RuntimeError:
        #    pass
        #try:
        #    bw.addEntries("chr1", 900, values=values[12:15], span=20, step=30)
        #    assert(1==0)
        #except RuntimeError:
        #    pass

        #Add a few intervals on a new chromosome
        bw.addEntries(["chr2"]*3, starts[0:3], ends=ends[0:3], values=values[0:3])
        bw.close()
        #check md5sum, this is the simplest method to check correctness
        #h = hashlib.md5(open(oname, "rb").read()).hexdigest()
        #assert(h=="ef104f198c6ce8310acc149d0377fc16")
        #Clean up
        os.remove(oname)

    def doWriteEmpty(self):
        ofile = tempfile.NamedTemporaryFile(delete=False)
        oname = ofile.name
        ofile.close()
        bw = pybm.openfile(oname, "w", end="Y")
        bw.addHeader([("1", 1000000), ("2", 1500000)])
        bw.close()

        #check md5sum
        h = hashlib.md5(open(oname, "rb").read()).hexdigest()
        print(h)
        #assert(h=="361c600e5badf0b45d819552a7822937")

        #Ensure we can openfile and get reasonable results
        bw = pybm.openfile(oname)
        assert(bw.chroms() == {'1': 1000000, '2': 1500000})
        assert(bw.intervals("1") == None)
        assert(bw.getvalues("1", 0, 1000000) == [])
        assert(bw.stats("1", 0, 1000000, nBins=2) == [None, None])
        bw.close()

        #Clean up
        os.remove(oname)

    def doWriteNumpy(self):
        ofile = tempfile.NamedTemporaryFile(delete=False)
        oname = ofile.name
        ofile.close()
        bw = pybm.openfile(oname, "w", end="Y")
        bw.addHeader([("chr1", 100), ("chr2", 150), ("chr3", 200), ("chr4", 250)])
        chroms = np.array(["chr1"] * 2 + ["chr2"] * 2 + ["chr3"] * 2 + ["chr4"] * 2)
        starts = np.array([0, 10, 40, 50, 60, 70, 80, 90], dtype=np.int64)
        ends = np.array([5, 15, 45, 55, 65, 75, 85, 95], dtype=np.int64)
        values0 = np.array(np.random.random_sample(8), dtype=np.float64)
        bw.addEntries(chroms, starts, ends=ends, values=values0)
        bw.close()

        vals = [(x, y, z) for x, y, z in zip(starts, ends, values0)]
        bw = pybm.openfile(oname)
        assert(bw.chroms() == {'chr1': 100, 'chr2': 150, 'chr3': 200, 'chr4': 250})
        for idx1, chrom in enumerate(["chr1", "chr2", "chr3", "chr4"]):
            for idx2, tup in enumerate(bw.intervals(chrom)):
                assert(tup[0] == starts[2 * idx1 + idx2])
                assert(tup[1] == ends[2 * idx1 + idx2])
                assert(np.isclose(tup[2], values0[2 * idx1 + idx2]))
        bw.close()

        #Clean up
        os.remove(oname)

    def testAll(self):
        print("test all")
        bw = self.doOpen()
        self.doChroms(bw)
        if not self.fname.startswith("http"):
            self.doHeader(bw)
            self.doStats(bw)
            self.doSum(bw)
            self.doValues(bw)
            self.doIntervals(bw)
            self.doWrite(bw)
            self.doOpenWith()
            print("self.doOpenWith")
            self.doWrite2()
            print("self.doWrite2")
            self.doWriteEmpty()
            print("doWriteEmpty")
            self.doWriteNumpy()
        bw.close()

class TestLocal():
    def testFoo(self):
        blah = TestRemote()
        blah.fname = os.path.dirname(pybm.__file__) + "/pybmtest/test.mbw"
        blah.testAll()

print("xxxx")
tl=TestLocal()
tl.testFoo()

class TestNumpy():
    def testNumpy(self):
        import os
        if pybm.numpy == 0:
            return 0
        import numpy as np

        bw = pybm.openfile("/tmp/delete.bw", "w")
        bw.addHeader([("1", 1000)], maxZooms=0)
        # Type 0
        chroms = np.array(["1"] * 10)
        starts = np.array([0, 10, 20, 30, 40, 50, 60, 70, 80, 90], dtype=np.int64)
        ends = np.array([5, 15, 25, 35, 45, 55, 65, 75, 85, 95], dtype=np.int64)
        values0 = np.array(np.random.random_sample(10), dtype=np.float64)
        bw.addEntries(chroms, starts, ends=ends, values=values0)

        starts = np.array([100, 110, 120, 130, 140, 150, 160, 170, 180, 190], dtype=np.int64)
        ends = np.array([105, 115, 125, 135, 145, 155, 165, 175, 185, 195], dtype=np.int64)
        values1 = np.array(np.random.random_sample(10), dtype=np.float64)
        bw.addEntries(chroms, starts, ends=ends, values=values1)

        # Type 1, single chrom, multiple starts/values, single span
        starts = np.array([200, 210, 220, 230, 240, 250, 260, 270, 280, 290], dtype=np.int64)
        values2 = np.array(np.random.random_sample(10), dtype=np.float64)
        bw.addEntries(np.str("1"), starts, span=np.int(8), values=values2)

        starts = np.array([300, 310, 320, 330, 340, 350, 360, 370, 380, 390], dtype=np.int64)
        values3 = np.array(np.random.random_sample(10), dtype=np.float64)
        bw.addEntries(np.str("1"), starts, span=np.int(8), values=values3)

        # Type 2, single chrom/start/span/step, multiple values
        values4 = np.array(np.random.random_sample(10), dtype=np.float64)
        bw.addEntries(np.str("1"), np.int(400), span=np.int(8), step=np.int64(2), values=values4)

        values5 = np.array(np.random.random_sample(10), dtype=np.float64)
        bw.addEntries(np.str("1"), np.int(500), span=np.int(8), step=np.int64(2), values=values5)

        bw.close()

        bw = pybm.openfile("/tmp/delete.bw")
        assert(bw is not None)

        def compy(start, v2):
            v = []
            for t in bw.intervals("1", start, start + 100):
                v.append(t[2])
            v = np.array(v)
            assert(np.all(np.abs(v - v2) < 1e-5))

        compy(0, values0)
        compy(100, values1)
        compy(200, values2)
        compy(300, values3)
        compy(400, values4)
        compy(500, values5)

        # Get values as a numpy array
        foo = bw.getvalues("1", 0, 100, numpy=False)
        assert(isinstance(foo, list))
        foo = bw.getvalues("1", 0, 100, numpy=True)
        assert(isinstance(foo, np.ndarray))

        bw.close()
        os.remove("/tmp/delete.bw")

    def testNumpyValues(self):
        if pybm.numpy == 0:
            return 0
        import numpy as np

        fname = "http://raw.githubusercontent.com/dpryan79/pybm/master/pybmTest/test.bw"
        bw = pybm.openfile(fname, "r")

        assert np.allclose(
            bw.getvalues("1", 0, 20, numpy=True),
            np.array(bw.getvalues("1", 0, 20), dtype=np.float32),
            equal_nan=True
        )

        assert np.allclose(
            bw.stats("1", 0, 20, "mean", 5, numpy=True),
            np.array(bw.stats("1", 0, 20, "mean", 5), dtype=np.float64),
            equal_nan=True
        )
