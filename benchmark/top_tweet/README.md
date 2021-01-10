# Top Tweet Benchmark

The top_tweet benchmark finds the most-retweeted tweet in a twitter API response.

## Purpose

This scenario tends to measure an implementation's laziness: its ability to avoid parsing unneeded
values, without knowing beforehand which values are needed.

To find the top tweet, an implementation needs to iterate through all tweets, remembering which one
had the highest retweet count. While it scans, it will find many "candidate" tweets with the highest
retweet count *up to that point.* However, While the implementation iterates through tweets, it will
have many "candidate" tweets. Essentially, it has to keep track of the "top tweet so far" while it
searches. However, only the text and screen_name of the *final* top tweet need to be parsed.
Therefore, JSON parsers that can only parse values on the first pass (such as DOM or streaming
parsers) will be forced to parse text and screen_name of every candidate (if not every single
tweet). Parsers which can delay parsing of values until later will therefore shine in scenarios like
this.

## Rules

The benchmark will be called with `run(padded_string &json, int64_t max_retweet_count, top_tweet_result &result)`.
The benchmark must:
- Find the tweet with the highest retweet_count at the top level of the "statuses" array.
- Find the *last* such tweet: if multiple tweets have the same top retweet_count, the last one
  should be returned.
- Exclude tweets with retweet_count above max_retweet_count. This restriction is solely here because
  the default twitter.json has a rather high retweet count in the third tweet, and to test laziness
  the matching tweet needs to be further down in the file.
- Fill in top_tweet_result with the corresponding fields from the matching tweet.

### Abridged Schema

The abridged schema (objects contain more fields than listed here):

```json
{
  "statuses": [
    {
      "text": "i like to tweet", // text containing UTF-8 and escape characters
      "user": {
        "screen_name": "AlexanderHamilton" // string containing UTF-8 (and escape characters?)
      },
      "retweet_count": 2, // uint32
    },
    ...
  ]
}
```
