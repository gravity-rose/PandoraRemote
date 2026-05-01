from dataclasses import dataclass


@dataclass(frozen=True)
class Track:
    station: str
    artist: str
    song: str


DEFAULT_PLAYLIST: list[Track] = [
    Track("Today's Hits", "Taylor Swift", "Anti-Hero"),
    Track("Today's Hits", "The Weeknd", "Blinding Lights"),
    Track("Today's Hits", "Dua Lipa", "Levitating"),
    Track("Chill Vibes", "Khruangbin", "Time (You and I)"),
    Track("Chill Vibes", "Tame Impala", "Let It Happen"),
    Track("Chill Vibes", "Mac DeMarco", "Chamber of Reflection"),
    Track("Classic Rock", "Led Zeppelin", "Kashmir"),
    Track("Classic Rock", "Pink Floyd", "Comfortably Numb"),
    Track("Classic Rock", "Queen", "Bohemian Rhapsody"),
]
