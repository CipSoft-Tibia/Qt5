(:*******************************************************:)
(: Test: K-SeqExprCast-1004                              :)
(: Written by: Frans Englich                             :)
(: Date: 2007-11-22T11:31:22+01:00                       :)
(: Purpose: A simple test exercising the whitespace facet for type xs:gYear. :)
(:*******************************************************:)
xs:gYear("
	 1999
	 ")
        eq
        xs:gYear("
	 1999
	 ")
      